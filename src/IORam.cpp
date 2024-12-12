#include "IORam.h"

IORam::IORam(sc_module_name nm, size_t size)
    : sc_module(nm),
      mem_(size),

      clk_i("clk_i"), addr_bi("addr_bi"), data_bi("data_bi"),
      data_bo("data_bo"), wr_i("wr_i"), rd_i("rd_i") {
  data_bo.initialize(0);

  SC_METHOD(bus_write);
  sensitive << clk_i.pos();

  SC_METHOD(bus_read);
  sensitive << clk_i.pos();
}

IORam::~IORam() {}

void IORam::bus_read() {
  if (wr_i.read())
    mem_[addr_bi.read()] = data_bi.read();
}

void IORam::bus_write() {
  if (rd_i.read())
    data_bo.write(mem_[addr_bi.read()]);
}
