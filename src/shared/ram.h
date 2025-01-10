#ifndef _MEM_H
#define _MEM_H

#include "tinyint.h"
#include <systemc>
#include <vector>

SC_MODULE(RandomAccessMemory) {
  sc_core::sc_in<bool> clk_i;
  sc_core::sc_in<u32> addr_bi;
  sc_core::sc_in<u32> data_bi;
  sc_core::sc_in<bool> wr_i;
  sc_core::sc_in<bool> rd_i;

  sc_core::sc_out<u32> data_bo;

  RandomAccessMemory(sc_core::sc_module_name nm, size_t size)
      : sc_module(nm), mem_(size),

        clk_i("clk_i"), addr_bi("addr_bi"), data_bi("data_bi"),
        data_bo("data_bo"), wr_i("wr_i"), rd_i("rd_i") {
    data_bo.initialize(0);

    SC_METHOD(bus_write);
    sensitive << clk_i.pos();

    SC_METHOD(bus_read);
    sensitive << clk_i.pos();
  }

  void set(int i, u32 val) {
    mem_[i] = val;
  }

  void bus_write() {
    if (wr_i.read())
      mem_[addr_bi.read()] = data_bi.read();
  }

  void bus_read() {
    if (rd_i.read())
      data_bo.write(mem_[addr_bi.read()]);
  }

private:
  std::vector<uint32_t> mem_;
};

#endif
