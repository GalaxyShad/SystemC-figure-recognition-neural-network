#ifndef __OVS_ITMO_LAB_SRC_SHARED_MEMORY_BUS_H_
#define __OVS_ITMO_LAB_SRC_SHARED_MEMORY_BUS_H_

#include <systemc>
#include "tinyint.h"

struct MemoryBus {
    sc_core::sc_out<u32>  adr_o;

    struct {
        sc_core::sc_out<bool> rd_o;
        sc_core::sc_in<u32>   data_i;
    } data_read;

    struct {
        sc_core::sc_out<bool> wr_o;
        sc_core::sc_out<u32>  data_o;
    } data_write;

    u32 read(u32 addr, std::function<void()> wait) {
        u32 data;

        wait();
        adr_o.write(addr);
        data_read.rd_o.write(1);

        wait();
        data_read.rd_o.write(0);

        wait();
        data = data_read.data_i.read();

        return data;
    }

    void write(u32 addr, u32 data, std::function<void()> wait) {
        wait();
        adr_o.write(addr);
        data_write.data_o.write(data);
        data_write.wr_o.write(1);

        wait();
        data_write.wr_o.write(0);
    }
};

struct ReadOnlyMemoryBus {
    sc_core::sc_out<u32>  adr_o;

    struct {
        sc_core::sc_out<bool> rd_o;
        sc_core::sc_in<u32>   data_i;
    } data_read;

    u32 read(u32 addr, std::function<void()> wait) {
        u32 data;

        wait();
        adr_o.write(addr);
        data_read.rd_o.write(1);

        wait();
        data_read.rd_o.write(0);

        wait();
        data = data_read.data_i.read();

        return data;
    }
};

#endif // __OVS_ITMO_LAB_SRC_SHARED_MEMORY_BUS_H_