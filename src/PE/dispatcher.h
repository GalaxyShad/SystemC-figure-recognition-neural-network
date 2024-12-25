#ifndef __OVS_ITMO_LAB_SRC_PE_DISPATCHER_H_
#define __OVS_ITMO_LAB_SRC_PE_DISPATCHER_H_

#include "sysc/communication/sc_signal_ports.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_simcontext.h"
#include "systemc"

#include "shared/memory_bus.h"

#include "pe.h"
#include <format>


SC_MODULE(Dispatcher) {

public:
    // Входы
    sc_core::sc_in<bool> clk_i;                  // Синхросигнал

    // Выходы
    sc_core::sc_out<bool> is_ready_o;            // Флаг готовности к работе

    ReadOnlyMemoryBus net_config_rom_bus;
    MemoryBus ram_bus;

    struct PeBus {
        sc_core::sc_in<bool> is_done_i;
        sc_core::sc_in<u32> result_neuron_i;
        
        sc_core::sc_out<u32> data_neuron_o;
        sc_core::sc_out<u32> data_weight_o;
        sc_core::sc_out<u32> out_neuron_index_o;
        sc_core::sc_out<bool> compute_neuron_o;
        sc_core::sc_out<bool> rst_o;
    };

    std::vector<PeBus> pe_bus;

private:
    std::vector<PeCore> pe_cores_;

    
public: 
    Dispatcher(sc_core::sc_module_name name) 
        : ::sc_core::sc_module(name), pe_bus(4)
    {
        is_ready_o.initialize(0);

        SC_METHOD(cycle);
        sensitive << clk_i.pos();

        SC_METHOD(on_data_net_config_change);
        sensitive << net_config_rom_bus.data_read.data_i;
    }

private:
    u32 current_layer_ = 1;

    u32 layers_count_ = 0;
    u32 prev_layer_neurons_count_ = 0;
    u32 curr_layer_neurons_count_ = 0;
    u32 weights_data_size_ = 0;

    u32 readed_count_ = 0;
    u32 computed_neurons_ = 0;

    bool tickusha_ = true;

    enum class Stage {
        LoadNetConfig,
        LoadNetInputLayerCount,
        LoadLayerDataRd,
        LoadLayerData,
        SendLayerDataToPe,
        SendLayerNeuronsData,
        ComputeLayer,
        Finished
    };

    Stage stage_ = Stage::LoadNetConfig;

private:
    static constexpr u32 LAYERS_COUNT_ADR = 0x0000'0000;
    static constexpr u32 LAYERS_NEURON_COUNT_ADR = LAYERS_COUNT_ADR + 1;
    
private:
    u32 read_from_net_config(u32 adr) {
        u32 data;

        
        data = net_config_rom_bus.data_read.data_i.read();

        return data;
    }

    u32 read_from_ram(u32 adr) {
        u32 data;

        ram_bus.adr_o.write(adr);
        ram_bus.data_read.rd_o.write(1);

        data = ram_bus.data_read.data_i.read();

        return data;
    }

    void stage_load_net_config() {
        net_config_rom_bus.data_read.rd_o.write(1);

        net_config_rom_bus.adr_o.write(LAYERS_COUNT_ADR);
    }

    void stage_load_net_input_layer() {
        net_config_rom_bus.adr_o.write(LAYERS_NEURON_COUNT_ADR);

        current_layer_ = 1;
    }

    void on_data_net_config_change() {
        
        auto data = net_config_rom_bus.data_read.data_i->read();

        if (layers_count_ == 0 && data == 0)
            return;

        switch (stage_) {
            case Stage::LoadNetConfig:
                layers_count_ = data;

                stage_ = Stage::LoadNetInputLayerCount;
            break;

            case Stage::LoadNetInputLayerCount:
                curr_layer_neurons_count_ = data;

                stage_ = Stage::LoadLayerData;
            break;

            case Stage::LoadLayerData:
                prev_layer_neurons_count_ = curr_layer_neurons_count_;
                curr_layer_neurons_count_ = data;

                weights_data_size_ = prev_layer_neurons_count_ * curr_layer_neurons_count_;

                stage_ = Stage::SendLayerDataToPe;
            break;
        }
    }

    void stage_load_layer_data() {
        net_config_rom_bus.data_read.rd_o.write(1);
        net_config_rom_bus.adr_o.write(LAYERS_NEURON_COUNT_ADR + current_layer_);

        readed_count_ = 0;

        for (auto& i : pe_bus) {
            i.compute_neuron_o->write(false);
            i.rst_o->write(true);
        }
    }

    void stage_send_layer_data_to_pe() {
        for (auto& i : pe_bus) {
            i.rst_o->write(false);
        }

        auto neuron = read_from_ram(readed_count_);
        auto weight = read_from_ram(LAYERS_NEURON_COUNT_ADR + layers_count_ + readed_count_);

        if (readed_count_ >= weights_data_size_) {
            for (auto& i : pe_bus) {
                i.compute_neuron_o->write(true);
                // i.rst_o->write(true);
            }
            stage_ = Stage::ComputeLayer;
        }

        readed_count_++;
    }

    void stage_compute() {
        for (auto& i : pe_bus) {
            i.rst_o->write(false);
        }

        for (auto& i : pe_bus) {
            if (i.is_done_i->read()) {
                std::cout << std::format(
                    "Neuorn {}, Result {}", 
                    i.out_neuron_index_o->read(),
                    *(float*)&i.result_neuron_i->read()
                ) << std::endl;

                i.out_neuron_index_o->write(computed_neurons_);
                i.rst_o->write(true);
                computed_neurons_++;
            }

            if (computed_neurons_ >= curr_layer_neurons_count_) {
                sc_core::sc_stop();
            }
            // i.rst_o->write(true);
        }
    }

    void cycle() {
        switch (stage_) {
            case Stage::LoadNetConfig:
                stage_load_net_config();
                break;
            case Stage::LoadNetInputLayerCount:
                stage_load_net_input_layer();
                break;
            case Stage::LoadLayerData:
                stage_load_layer_data(); break;
            case Stage::SendLayerDataToPe:
                stage_send_layer_data_to_pe();break;
            case Stage::ComputeLayer:
                stage_compute();break;
            
            break;
        }
    }

    void main_thread() {
        while (1) { cycle(); }
    }

};

#endif // __OVS_ITMO_LAB_SRC_PE_DISPATCHER_H_