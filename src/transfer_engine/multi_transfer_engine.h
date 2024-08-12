#ifndef MULTI_TRANSFER_ENGINE_H_
#define MULTI_TRANSFER_ENGINE_H_

#include <asm-generic/errno-base.h>
#include <cstdint>
#include <limits.h>
#include <string.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "transfer_engine/transfer_engine.h"
#include "transfer_engine/transfer_metadata.h"
#include "transport.h"

namespace mooncake
{
    struct MultiTransferEngine
    {
        struct SegmentDesc
        {
            int type; // RDMA / NVMeoF
            union
            {
                struct
                {
                    void *addr;
                    uint64_t size;
                    const char* location;
                } rdma;
                struct
                {
                    const char* file_path;
                    const char *subsystem_name;
                    const char *proto;
                    const char *ip;
                    uint64_t port;
                    // maybe more needed for mount
                } nvmeof;
            } desc_;
        };

    public:
        MultiTransferEngine(const char *name, std::shared_ptr<TransferMetadata> meta)
            : local_name(name), meta(meta) {}

        int freeEngine()
        {
            while (!installed_transports.empty())
            {
                if (uninstallTransport(installed_transports.back()) < 0)
                {
                    return -1;
                }
            }
            return 0;
        }

        Transport *installTransport(const std::string& proto, const std::string& path_prefix, 
                              const std::string& local_name, std::shared_ptr<TransferMetadata> meta,
                             void** args)
        {
            if (findName(path_prefix.c_str()) != NULL)
            {
                errno = EEXIST;
                return NULL;
            }
            Transport *xport = initTransport(path_prefix.c_str());
            if (!xport)
            {
                errno = ENOMEM;
                return NULL;
            }

            if (xport->install(local_name, meta, args) < 0)
            {
                goto fail;
            }
            installed_transports.emplace_back(xport);
            return xport;
        fail:
            delete xport;
            return NULL;
        }

        int uninstallTransport(Transport *xport)
        {
            for (auto it = installed_transports.begin(); it != installed_transports.end(); ++it)
            {
                if (*it == xport)
                {
                    delete xport;
                    installed_transports.erase(it);
                    return 0;
                }
            }
            errno = EINVAL;
            return -1;
        }

        int registerSegment(const SegmentDesc &seg_desc) {
            // TODO
            return 0;
        }
        int unregisterSegment(const SegmentDesc &seg_desc) {
            // TODO
            return 0;
        }

        std::pair<SegmentID, Transport *> openSegment(const char *path)
        {
            return std::make_pair(1, installed_transports[0]);
        //     const char *pos = NULL;
        //     if (path == NULL || (pos = strchr(path, ':')) == NULL)
        //     {
        //         errno = EINVAL;
        //         return std::make_pair(-1, nullptr);
        //     }

        //     auto xport = findName(path, pos - path);
        //     if (!xport)
        //     {
        //         errno = ENOENT;
        //         return std::make_pair(-1, nullptr);
        //     }

        //     auto seg_id = xport->openSegment(pos + 1);
        //     if (seg_id < 0)
        //     {
        //         goto fail;
        //     }
        //     return std::make_pair(seg_id, xport);

        // fail:
        //     return std::make_pair(-1, nullptr);
        }

        int closeSegment(SegmentID seg_id)
        {
            if (seg_id < 0)
            {
                errno = EINVAL;
                return -1;
            }
            // TODO: get xport
            // if (xport->closeSegment(seg_id) < 0)
            // {
            //     return -1;
            // }
            return 0;
        }

        string local_name;
        std::shared_ptr<TransferMetadata> meta;

    private:

        Transport *
        findName(const char *name, size_t n = SIZE_MAX)
        {
            for (const auto &xport : installed_transports)
            {
                if (strncmp(xport->getName(), name, n) == 0)
                {
                    return xport;
                }
            }
            return NULL;
        }

        Transport *initTransport(const char *proto)
        {
            // TODO: return a transport object according to protocol
            if (std::string(proto) == "dummy")
            {
                return new DummyTransport();
            }
        }

        std::vector<Transport *> installed_transports;
    };
}

#endif