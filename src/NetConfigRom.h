#ifndef MYSYSTEMCPROJECT_NETCONFIGROM_H
#define MYSYSTEMCPROJECT_NETCONFIGROM_H

#include "../Pure-CPP20-Neural-Network/src/neural_network_model.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_module_name.h"
#include "systemc.h"

SC_MODULE(NetConfigRom) {
  sc_in<bool> clk_i;
  sc_in<uint32_t> addr_bi;
  sc_in<bool> rd_i;

  sc_out<uint32_t> data_bo;

  explicit NetConfigRom(sc_module_name name, NeuralNetworkModel & nn_model) : ::sc_core::sc_module(name) {
    auto layers_count = nn_model.layers_count();
    auto layers_size_list = nn_model.layers_sizes_vector();

    mem_.push_back(layers_count);

    for (auto i : layers_size_list) {
      mem_.push_back(i);
    }

    for (auto &m : nn_model.weights()) {
      for (int out = 0; out < m.out_count(); out++) {
        for (int in = 0; in < m.in_count(); in++) {
          float weight = m.weight_between(out, in);
          uint32_t float_raw = *((uint32_t *)(&weight));

          mem_.push_back(float_raw);
        }
      }
    }

    data_bo.initialize(0);

    SC_METHOD(bus_read);
    sensitive << clk_i.pos();
  }

private:
  std::vector<uint32_t> mem_;

  inline void bus_read() {
    if (!rd_i.read())
      return;

    auto adr = addr_bi.read();
    sc_assert(adr < mem_.size());

    data_bo.write(mem_[adr]);
  }
};

#endif // MYSYSTEMCPROJECT_NETCONFIGROM_H
