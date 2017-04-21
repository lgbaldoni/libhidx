#include <libhidx/Interface.hh>

#include <buffer.pb.h>
#include <libhidx/InterfaceHandle.hh>
#include <libhidx/Parser.hh>

#include <iostream>
#include <cassert>

namespace libhidx {

    Interface::Interface(const buffer::Interface& interface, Device& device) : m_interface{interface.altsetting(0)}, m_device{device}, readingRuns{false}, stopReadingRequest{false} {
        for(const auto& endpoint: m_interface.endpoint()){
            // TODO: remove magic constants
            bool isInterrupt =
                (endpoint.attributes() & 0x03) == 3;
            bool isInput =
                (endpoint.endpointaddress() & 0x80) == 0x80;
            bool isOutput =
                (endpoint.endpointaddress() & 0x80) == 0x00;

            if(isInterrupt && isInput){
                m_inputAddress = endpoint.endpointaddress();
                m_inputMaxSize = endpoint.maxpacketsize();
                break;
            }

            if(isInterrupt && isOutput){
                m_outputAddress = endpoint.endpointaddress();
                m_hasOutputAddress = true;
                break;
            }
        }
    }

    Interface::~Interface() {
        stopReading();
    }

    bool Interface::isHid() const {
        return m_interface.interfaceclass() == 3;
    }

    hid::Item& Interface::getHidReportDesc() {
        assert(isHid());

        if(m_hidReportDesc){
            return *m_hidReportDesc.get();
        }

        constexpr uint16_t bufferLength = 1024;

        auto handle = getHandle();
        constexpr auto LIBUSB_REQUEST_GET_DESCRIPTOR = 0x06;
        constexpr auto LIBUSB_DT_REPORT = 0x22;
        auto data = handle->controlInTransfer(0x81, LIBUSB_REQUEST_GET_DESCRIPTOR, ((LIBUSB_DT_REPORT << 8) | 0),
                                            static_cast<uint16_t>(m_interface.interfacenumber()), bufferLength, 1000);

        std::string buffer;
        int size;
        std::tie(size, buffer) = data;

        if(data.first <= 0){
            throw ConnectionException{"Libusb control transfer failed: " + std::to_string(data.first)};
        }

        auto parser = Parser{reinterpret_cast<const uint8_t*>(buffer.data()), static_cast<size_t>(size)};

        auto rootItem = parser.parse();
        m_hidReportDesc.reset(rootItem);

        return *m_hidReportDesc.get();
    }

    std::string Interface::getName() const {
        auto name = std::string{};
        const auto devStrings = m_device.getStrings();

        name += devStrings.manufacturer;
        name += " ";
        name += devStrings.product;

        name += " (interface ";
        name += std::to_string(getNumber());
        name += ")";

        return name;
    }

    void Interface::beginReading() {
        if(readingRuns) {
            std::cerr << "fail begin reading" << std::endl;
            return;
        }
        if(readingThread.joinable()){
            readingThread.join();
        }

        readingRuns = true;
        std::thread t{&Interface::runner, this};

        readingThread = std::move(t);
    }

    void Interface::stopReading() {
        if(readingRuns) {
            stopReadingRequest = true;
            readingThread.join();
        }
    }

    void Interface::runner() {
        std::cerr << "thread running" << std::endl;

        auto handle = getHandle();

        while(!stopReadingRequest){
            auto response = handle->interruptInTransfer(
                static_cast<uint8_t>(m_inputAddress),
                static_cast<uint16_t>(m_inputMaxSize),
                1000
            );

            constexpr auto LIBUSB_ERROR_TIMEOUT = -7;

            if(response.retvalue() == 0) {
                std::vector<unsigned char> data{std::begin(response.data()), std::end(response.data())};
                updateData(std::move(data));
                if(m_listener) {
                    m_listener();
                }
            } else if(response.retvalue() == LIBUSB_ERROR_TIMEOUT){
                std::cerr << "Interrupt transfer timeout" << std::endl;
            } else {
                std::cerr << "Interrupt transfer fail" << std::endl;
            }
        }

        stopReadingRequest = false;
        readingRuns = false;
    }

    std::shared_ptr<InterfaceHandle> Interface::getHandle() {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock{mtx};

        std::shared_ptr<InterfaceHandle> ptr;
        if(m_handle.expired()){
            ptr.reset(new InterfaceHandle{*this});
            m_handle = ptr;
        }

        return m_handle.lock();
    }

    void Interface::setReadingListener(std::function<void()> listener) {
        m_listener = listener;
    }

    void Interface::updateData(std::vector<unsigned char>&& dataRef) {
        auto& reportDesc = getHidReportDesc();

        auto data = std::move(dataRef);
        unsigned reportId = 0;

        if(reportDesc.isNumbered()){
            reportId = data.front();
            data.erase(std::begin(data));
        }

        reportDesc.forEach([&data, reportId](hid::Item* item){
            auto control = dynamic_cast<hid::Control*>(item);
            if(!control){
                return;
            }
            control->setData(data, reportId);
        });
    }

    void Interface::sendData() {
        std::vector<unsigned char> data;
        m_hidReportDesc->forEach([&data](auto item){
            auto control = dynamic_cast<hid::Control*>(item);
            if(!control){
                return;
            }
            if(!control->getUsages().size()){
                return;
            }
            if(control->getReportType() != hid::Control::Type::OUTPUT){
                return;
            }

            auto controlData = control->getData();
            auto controlOffset = control->getOffset();
            auto controlSize = control->getSize() * control->getCount();

            for(unsigned i = 0; i < controlSize; ++i){
                auto offset = controlOffset + i;
                auto bytePos = offset / 8U;
                auto bitPos = offset % 8U;
                auto bit = (controlData >> i) & 1;

                if(bytePos >= data.size()){
                    data.resize(bytePos + 1);
                }

                data[bytePos] |= bit << bitPos;
            }

        });

        sendOutputReport(data);
    }

    void Interface::sendOutputReport(const std::vector<unsigned char>& data) {
        auto handle = getHandle();
        if(m_hasOutputAddress) {
            handle->interruptOutTransfer(
                    static_cast<unsigned char>(m_outputAddress),
                    data.data(),
                    data.size(),
                    1000
            );
        } else {
            constexpr auto LIBUSB_REQUEST_TYPE_CLASS = (0x01 << 5);
            constexpr auto LIBUSB_RECIPIENT_INTERFACE = 0x01;
            constexpr auto LIBUSB_ENDPOINT_OUT = 0x00;
            handle->controlOutTransfer(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT, 0x09,
                                    (2/*HID output*/ << 8) | /*report_number*/ 0,
                                    static_cast<uint16_t>(m_interface.interfacenumber()), data.data(), data.size(), 500);
        }
    }

    uint32_t Interface::getNumber() const {
        return m_interface.interfacenumber();
    }


}
