#include "session.hpp"

#include "auth_utils.hpp"
#include "logger.hpp"
#include "rpc.hpp"

Session::Session() {
    setMessageHandler(Base::MessageHandler::create<Session, &Session::handleRpcMessage>(*this));
    random = create_secret();
}

Session::~Session() {
    APP_LOGI("BLE: Session destroyed");
}

void Session::handleRpcMessage(const MessageBuffer& message, MessageBuffer& response) {
    if (!authenticated && rpc.needs_authentication(message)) {
        RpcDispatcher::Response writer(response);
        writer.write_error("Not authenticated");
        return;
    }

    rpc.dispatch(message, response, *this);
}
