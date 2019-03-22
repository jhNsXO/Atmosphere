/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "fs_results.hpp"

enum FsIFileCmd : u32 {
    FsIFileCmd_Read = 0,
    FsIFileCmd_Write = 1,
    FsIFileCmd_Flush = 2,
    FsIFileCmd_SetSize = 3,
    FsIFileCmd_GetSize = 4,
    FsIFileCmd_OperateRange = 5,
};
    
class IFile {
    public:
        virtual ~IFile() {}
        
        Result Read(uint64_t *out, uint64_t offset, void *buffer, uint64_t size, uint32_t flags) {
            (void)(flags);
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            if (size == 0) {
                *out = 0;
                return 0;
            }
            if (buffer == nullptr) {
                return ResultFsNullptrArgument;
            }
            return ReadImpl(out, offset, buffer, size);
        }
        
        Result GetSize(uint64_t *out) {
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetSizeImpl(out);
        }
        
        Result Flush() {
            return FlushImpl();
        }
        
        Result Write(uint64_t offset, void *buffer, uint64_t size, uint32_t flags) {
            if (size == 0) {
                return 0;
            }
            if (buffer == nullptr) {
                return ResultFsNullptrArgument;
            }
            const bool flush = (flags & 1) != 0;
            return WriteImpl(offset, buffer, size, flush);
        }
        
        Result Write(uint64_t offset, void *buffer, uint64_t size) {
            return Write(offset, buffer, size, false);
        }
        
        Result SetSize(uint64_t size) {
            return SetSizeImpl(size);
        }
        
        Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) {
            if (operation_type != 3) {
                return OperateRangeImpl(operation_type, offset, size, out_range_info);
            }
            return ResultFsUnsupportedOperation;
        }
        
    protected:
        /* ...? */
    private:
        virtual Result ReadImpl(u64 *out, u64 offset, void *buffer, u64 size) = 0;
        virtual Result GetSizeImpl(u64 *out) = 0;
        virtual Result FlushImpl() = 0;
        virtual Result WriteImpl(u64 offset, void *buffer, u64 size, bool flush) = 0;
        virtual Result SetSizeImpl(u64 size) = 0;
        virtual Result OperateRangeImpl(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) = 0;
};

class IFileInterface : public IServiceObject {
    private:
        std::unique_ptr<IFile> base_file;
    public:
        IFileInterface(IFile *f) : base_file(f) {
            /* ... */
        };
        IFileInterface(std::unique_ptr<IFile> f) : base_file(std::move(f)) {
            /* ... */
        };

    private:
        /* Actual command API. */
        virtual Result Read(OutBuffer<u8, BufferType_Type1> buffer, Out<u64> out_read, u64 offset, u64 size, u32 read_flags) final {
            return this->base_file->Read(out_read.GetPointer(), offset, buffer.buffer, std::min(buffer.num_elements, size), read_flags);
        };
        virtual Result Write(InBuffer<u8, BufferType_Type1> buffer, u64 offset, u64 size, u32 write_flags) final {
           return this->base_file->Write(offset, buffer.buffer, std::min(buffer.num_elements, size), write_flags);
        };
        virtual Result Flush() final {
            return this->base_file->Flush();
        };
        virtual Result SetSize(u64 size) final {
            return this->base_file->SetSize(size);
        };
        virtual Result GetSize(Out<u64> size) final {
            return this->base_file->GetSize(size.GetPointer());
        };
        virtual Result OperateRange(Out<FsRangeInfo> range_info, u32 operation_type, u64 offset, u64 size) final {
            return this->base_file->OperateRange(operation_type, offset, size, range_info.GetPointer());
        };
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0- */
            MakeServiceCommandMeta<FsIFileCmd_Read, &IFileInterface::Read>(),
            MakeServiceCommandMeta<FsIFileCmd_Write, &IFileInterface::Write>(),
            MakeServiceCommandMeta<FsIFileCmd_Flush, &IFileInterface::Flush>(),
            MakeServiceCommandMeta<FsIFileCmd_SetSize, &IFileInterface::SetSize>(),
            MakeServiceCommandMeta<FsIFileCmd_GetSize, &IFileInterface::GetSize>(),
            
            /* 4.0.0- */
            MakeServiceCommandMeta<FsIFileCmd_OperateRange, &IFileInterface::OperateRange, FirmwareVersion_400>(),
        };
};