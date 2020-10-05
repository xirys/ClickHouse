#pragma once

#include <IO/WriteBuffer.h>
#include <IO/BufferWithOwnMemory.h>
#include <common/types.h>
#include <grpc++/server.h>
#include "clickhouse_grpc.grpc.pb.h"

namespace DB
{
class WriteBufferFromGRPC : public BufferWithOwnMemory<WriteBuffer>
{
public:
    using GRPCQueryInfo = clickhouse::grpc::QueryInfo;
    using GRPCResult = clickhouse::grpc::Result;

    WriteBufferFromGRPC(
        grpc::ServerAsyncReaderWriter<GRPCResult, GRPCQueryInfo> * responder_,
        void * tag_,
        std::function<GRPCResult(const String & buffer)> set_response_details_)
        : responder(responder_), tag(tag_), set_response_details(set_response_details_)
    {
    }

    ~WriteBufferFromGRPC() override {}
    bool onProgress() { return progress; }
    bool isFinished() { return finished; }
    void setFinish(bool fl) { finished = fl; }
    void setResponse(std::function<GRPCResult(const String & buffer)> function) { set_response_details = function; }
    void finalize() override
    {
        progress = false;
        finished = true;
        responder->Finish(grpc::Status(), tag);
    }

protected:
    grpc::ServerAsyncReaderWriter<GRPCResult, GRPCQueryInfo> * responder;
    void * tag;

    bool progress = false;
    bool finished = false;
    std::function<GRPCResult(const String & buffer)> set_response_details;


    void nextImpl() override
    {
        progress = true;

        String buffer(working_buffer.begin(), working_buffer.begin() + offset());
        auto response = set_response_details(buffer);
        responder->Write(response, tag);
    }
};
}
