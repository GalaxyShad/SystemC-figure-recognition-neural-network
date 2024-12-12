#ifndef _MEM_H
#define _MEM_H

#include "systemc.h"

SC_MODULE(IORam) {
  sc_in<bool> clk_i;
  sc_in<int> addr_bi;
  sc_in<int> data_bi;
  sc_in<bool> wr_i;
  sc_in<bool> rd_i;

  sc_out<int> data_bo;

  IORam(sc_module_name nm, size_t size);
  ~IORam();

  void bus_write();
  void bus_read();

private:
  std::vector<uint32_t> mem_;
};

#endif
