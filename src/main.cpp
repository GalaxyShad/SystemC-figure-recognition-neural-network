
#include "NetConfigRom.h"
#include "PE/dispatcher.h"
#include "PE/pe.h"

#include "../Pure-CPP20-Neural-Network/src/utils.h"
#include "sysc/communication/sc_signal.h"
#include <format>
#include <limits>
#include <vector>

struct PeSignals {
    sc_signal<bool> is_done_i;
    sc_signal<u32> result_neuron_i;
    
    sc_signal<u32> data_neuron_o;
    sc_signal<u32> data_weight_o;
    sc_signal<u32> out_neuron_index_o;
    sc_signal<bool> compute_neuron_o;
    sc_signal<bool> rst_o;
};

int sc_main(int argc, char *argv[]) {

    std::string model_filename; 
    // model_filename = "figures-model_o#^_-49-14-3-_x5each.bin";
    model_filename = "figures-model_o#^_-49-3-_x5each.bin";

    auto path = "Pure-CPP20-Neural-Network/" + model_filename;

    auto neural_net_model_serialized = load_binary(path.c_str());
    auto neural_net_model = NeuralNetworkModel::deserialize(neural_net_model_serialized);

    constexpr int CORES_COUNT = 1;

    NetConfigRom net_config("NNConfigRom", neural_net_model);
    RandomAccessMemory memory("memory", 256);
    Dispatcher dispatcher("dispatcher", CORES_COUNT);
    
    #define X 1
    // std::vector<int> a = {
    //     0, 0, 0, 0, 0, 0, 0,
    //     0, X, X, X, X, X, 0,
    //     0, X, 0, 0, 0, X, 0,
    //     0, X, 0, 0, 0, X, 0,
    //     0, X, 0, 0, 0, X, 0,
    //     0, X, X, X, X, X, 0,
    //     0, 0, 0, 0, 0, 0, 0,
    // };

    // std::vector<int> a = {
    //     0, 0, 0, 0, 0, 0, 0,
    //     0, 0, X, X, X, 0, 0,
    //     0, X, 0, 0, 0, X, 0,
    //     0, X, 0, X, 0, X, 0,
    //     0, X, 0, 0, 0, X, 0,
    //     0, 0, X, X, X, 0, 0,
    //     0, 0, 0, 0, 0, 0, 0,
    // };

    std::vector<int> a = {
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, X, 0, 0, 0,
        0, 0, X, 0, X, 0, 0,
        0, X, 0, 0, 0, X, 0,
        X, 0, 0, 0, 0, 0, X,
        X, X, X, X, X, X, X,
        0, 0, 0, 0, 0, 0, 0,
    };

    #undef X

    for (int i = 0; i < a.size(); i++) {
        auto data_f = (float)a[i];
        u32 data_u32 = *(u32*)&data_f;

        memory.set(i, data_u32);
    }

    sc_clock clk("clk", sc_time(10, SC_NS));
    sc_signal<bool> dispatcher_rdy;

    dispatcher.clk_i(clk);
    dispatcher.is_ready_o(dispatcher_rdy);

    // === DISPATCHER RAM ===
    sc_signal<u32> dispatcher_ram_addr;
    sc_signal<u32> dispatcher_ram_bo;
    sc_signal<u32> dispatcher_ram_bi;
    sc_signal<bool> dispatcher_ram_wr;
    sc_signal<bool> dispatcher_ram_rd;

    // DISPATCHER
    dispatcher.ram_bus.adr_o(dispatcher_ram_addr);
    dispatcher.ram_bus.data_read.data_i(dispatcher_ram_bi);
    dispatcher.ram_bus.data_read.rd_o(dispatcher_ram_rd);
    dispatcher.ram_bus.data_write.data_o(dispatcher_ram_bo);
    dispatcher.ram_bus.data_write.wr_o(dispatcher_ram_wr);
    
    // RAM
    memory.clk_i(clk);
    memory.addr_bi(dispatcher_ram_addr);
    memory.data_bi(dispatcher_ram_bo);
    memory.data_bo(dispatcher_ram_bi);
    memory.wr_i(dispatcher_ram_wr);
    memory.rd_i(dispatcher_ram_rd);

    // === DISPATCHER ROM === 
    sc_signal<u32> dispatcher_nn_config_addr;
    sc_signal<u32> dispatcher_nn_config_bi;
    sc_signal<bool> dispatcher_nn_config_rd;

    // DISPATCHER
    dispatcher.net_config_rom_bus.adr_o(dispatcher_nn_config_addr);
    dispatcher.net_config_rom_bus.data_read.data_i(dispatcher_nn_config_bi);
    dispatcher.net_config_rom_bus.data_read.rd_o(dispatcher_nn_config_rd);

    // ROM
    net_config.clk_i(clk);
    net_config.addr_bi(dispatcher_nn_config_addr);
    net_config.data_bo(dispatcher_nn_config_bi);
    net_config.rd_i(dispatcher_nn_config_rd);

    // === PE Cores === //
   

    std::vector<PeSignals> cores_signal(CORES_COUNT);
    std::vector<PeCore*> peCores;

    for (int i = 0; i < CORES_COUNT; i++) {
        std::string s = std::format("pe_core_{}", i);

        peCores.push_back(new PeCore(s.c_str(), 8192, 8192));

        dispatcher.pe_bus[i].rst_o(cores_signal[i].rst_o);
        dispatcher.pe_bus[i].compute_neuron_o(cores_signal[i].compute_neuron_o);
        dispatcher.pe_bus[i].data_neuron_o(cores_signal[i].data_neuron_o);
        dispatcher.pe_bus[i].data_weight_o(cores_signal[i].data_weight_o);
        dispatcher.pe_bus[i].out_neuron_index_o(cores_signal[i].out_neuron_index_o);
        dispatcher.pe_bus[i].result_neuron_i (cores_signal[i].result_neuron_i);
        dispatcher.pe_bus[i].is_done_i (cores_signal[i].is_done_i);

        peCores[i]->clk_i(clk);
        peCores[i]->rst_i(cores_signal[i].rst_o);
        peCores[i]->compute_mode_i(cores_signal[i].compute_neuron_o);
        peCores[i]->data_neurons_i(cores_signal[i].data_neuron_o);
        peCores[i]->data_weights_in_i(cores_signal[i].data_weight_o);
        peCores[i]->output_neuron_index_i(cores_signal[i].out_neuron_index_o);
        peCores[i]->result_neuron_o(cores_signal[i].result_neuron_i);
        peCores[i]->is_computation_end_o(cores_signal[i].is_done_i);
    }


    // sc_trace_file *wf = sc_create_vcd_trace_file("wave");
    // sc_trace(wf, clk, "clk");
    // sc_trace(wf, addr, "addr_bo");
    // sc_trace(wf, data_cpu_bi, "data_bi");
    // sc_trace(wf, data_cpu_bo, "data_bo");
    // sc_trace(wf, wr, "wr");
    // sc_trace(wf, rd, "rd");

    sc_start();


    // sc_close_vcd_trace_file(wf);

    int b = 1;

    return (0);

}
