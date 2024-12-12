#ifndef MYSYSTEMCPROJECT_PE_CORE_H
#define MYSYSTEMCPROJECT_PE_CORE_H

#include "systemc.h"

using AdrSize = uint32_t; // Размер шины адреса

SC_MODULE(PeCore) {
  sc_in<AdrSize> io_ram_addr_i;
  sc_in<bool> clk_i;

  sc_out<bool> ready_o;

  PeCore(sc_module_name name);
  ~PeCore();
};

SC_MODULE(DispatcherController) {
  sc_in<AdrSize> io_ram_addr_i;
  sc_in<bool> clk_i;

  DispatcherController(sc_module_name name);
  ~DispatcherController();
};

SC_MODULE(IORam) {
  sc_in<bool> clk_i;
  sc_in<int> addr_bi;
  sc_in<int> data_bi;
  sc_out<int> data_bo;
  sc_in<bool> wr_i;
  sc_in<bool> rd_i;

  IORam(sc_module_name nm);
  ~IORam();

  void bus_write();
  void bus_read();

private:
  std::vector<uint32_t> mem_;
};



SC_MODULE(FigureRecognitionMicrocontroller) {
  sc_in<bool> clk_i;

  FigureRecognitionMicrocontroller(
      size_t cores_count // кол-во ядер 3..7

      );
};

#endif // MYSYSTEMCPROJECT_PE_CORE_H
