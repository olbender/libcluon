/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OD4SESSION_HPP
#define OD4SESSION_HPP

#include "cluon/Time.hpp"
#include "cluon/ToProtoVisitor.hpp"
#include "cluon/UDPReceiver.hpp"
#include "cluon/UDPSender.hpp"
#include "cluon/cluon.hpp"
#include "cluon/cluonDataStructures.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace cluon {
/**
This class provides an interface to an OpenDaVINCI v4 session:

\code{.cpp}
cluon::OD4Session od4{111, [](cluon::data::Envelope &&envelope){ std::cout << "Received cluon::Envelope" << std::endl;}
};

// Do something in parallel.

MyMessage msg;
od4.send(msg);
\endcode
*/
class LIBCLUON_API OD4Session {
   private:
    OD4Session(const OD4Session &) = delete;
    OD4Session(OD4Session &&)      = delete;
    OD4Session &operator=(const OD4Session &) = delete;
    OD4Session &operator=(OD4Session &&) = delete;

   public:
    /**
     * Constructor.
     *
     * @param CID OpenDaVINCI v4 session identifier [1 .. 254]
     * @param delegate Function to call on newly arriving Envelopes.
     */
    OD4Session(uint16_t CID, std::function<void(cluon::data::Envelope &&envelope)> delegate = nullptr) noexcept;

    /**
     * This method will send a given Envelope to this OpenDaVINCI v4 session.
     *
     * @param envelope to be sent.
     */
    void send(cluon::data::Envelope &&envelope) noexcept;

    /**
     * This method will send a given message to this OpenDaVINCI v4 session.
     *
     * @param message Message to be sent.
     * @param sampleTimeStamp Time point when this sample to be sent was captured (default = sent time point).
     * @param senderStamp Optional sender stamp (default = 0).
     */
    template <typename T>
    void send(T &message, const cluon::data::TimeStamp &sampleTimeStamp = cluon::data::TimeStamp(), uint32_t senderStamp = 0) noexcept {
        cluon::ToProtoVisitor protoEncoder;

        cluon::data::Envelope envelope;
        {
            envelope.dataType(static_cast<int32_t>(message.ID()));
            message.accept(protoEncoder);
            envelope.serializedData(protoEncoder.encodedData());
            envelope.sent(cluon::time::now());
            envelope.sampleTimeStamp((0 == (sampleTimeStamp.seconds() + sampleTimeStamp.microseconds())) ? envelope.sent() : sampleTimeStamp);
            envelope.senderStamp(senderStamp);
        }

        send(std::move(envelope));
    }

   public:
    bool isRunning() noexcept;

    /**
     * This method transforms a given Envelope to a string representation to be
     * sent to an OpenDaVINCI session.
     *
     * @param envelope Envelope with payload to be sent.
     * @return String representation of the Envelope to be sent to OpenDaVINCI v4.
     */
    static std::string serializeAsOD4Container(cluon::data::Envelope &&envelope) noexcept;

   private:
    void callback(std::string &&data, std::string &&from, std::chrono::system_clock::time_point &&timepoint) noexcept;
    void sendInternal(std::string &&dataToSend) noexcept;

   private:
    cluon::UDPReceiver m_receiver;
    cluon::UDPSender m_sender;

    std::function<void(cluon::data::Envelope &&envelope)> m_delegate{nullptr};
};

} // namespace cluon
#endif
