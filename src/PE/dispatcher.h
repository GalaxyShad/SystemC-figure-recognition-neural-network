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
    Dispatcher(sc_core::sc_module_name name, int cores_count) 
        : ::sc_core::sc_module(name), pe_bus(cores_count)
    {
        is_ready_o.initialize(0);

        SC_METHOD(cycle);
        sensitive << clk_i.pos();
    }

private:
    u32 current_layer_ = 1;

    u32 layers_count_ = 0;
    u32 prev_layer_neurons_count_ = 0;
    u32 curr_layer_neurons_count_ = 0;
    u32 weights_data_size_ = 0;

    u32 readed_count_ = 0;
    u32 computed_neurons_ = 0;

    u32 cycles_to_wait = 0;

    u32 computed_neuron_buffer_ = 0;

    bool tickusha_ = true;

    enum class Stage {
        // Состояния инициализации
        ReadLayersCountRequest,
        ReadLayersCountRead,
        ReadInputLayerNeuronsCountRead,

        // Состояния чтения следующего слоя
        ReadCurrentLayerNeuronsCountRequest,
        ReadCurrentLayerNeuronsCountRead,

        ReadLayerDataRequest,
        ReadLayerDataRead,
        
        Compute

    };

    Stage stage_ = Stage::ReadLayersCountRequest;

private:
    static constexpr u32 LAYERS_COUNT_ADR = 0x0000'0000;
    static constexpr u32 LAYERS_NEURON_COUNT_ADR = LAYERS_COUNT_ADR + 1;
    
private:

    void cycle() {
        if (cycles_to_wait > 0) {
            cycles_to_wait--;
            return;
        }

        switch (stage_) {
        case Stage::ReadLayersCountRequest:
            stage_read_layers_count_req(); break;
        case Stage::ReadLayersCountRead:
            stage_read_layers_count_read(); break;
        case Stage::ReadInputLayerNeuronsCountRead:
            stage_read_input_layer_neurons_count(); break;
        case Stage::ReadCurrentLayerNeuronsCountRequest:
            stage_read_current_layer_neurons_count_request(); break;
        case Stage::ReadCurrentLayerNeuronsCountRead:
            stage_read_current_layer_neurons_count_read(); break;
        case Stage::ReadLayerDataRequest:
            stage_read_layer_data_request(); break;
        case Stage::ReadLayerDataRead:
            stage_read_layer_data_read(); break;
        case Stage::Compute:
            stage_compute(); break;
        }
    }

    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////

    // INFO на чтение данных из памяти уходит 3 такта
    // 1. выставление адреса и флага чтения памяти
    // 2. ожидание чтобы памяти отработала 
    // 3. чтение результата data шины памяти

    void stage_read_layers_count_req() {
        net_config_rom_bus.data_read.rd_o.write(1);
        net_config_rom_bus.adr_o.write(LAYERS_COUNT_ADR);

        stage_ = Stage::ReadLayersCountRead;

        cycles_to_wait = 1;
    }

    void stage_read_layers_count_read() {
        layers_count_ = net_config_rom_bus.data_read.data_i->read();

        net_config_rom_bus.adr_o.write(LAYERS_NEURON_COUNT_ADR);

        stage_ = Stage::ReadInputLayerNeuronsCountRead;

        cycles_to_wait = 1;
    }

    void stage_read_input_layer_neurons_count() {
        curr_layer_neurons_count_ = net_config_rom_bus.data_read.data_i->read();
    
        current_layer_ = 1;

        stage_ = Stage::ReadCurrentLayerNeuronsCountRequest;
    }

    void stage_read_current_layer_neurons_count_request() {
        net_config_rom_bus.adr_o.write(LAYERS_NEURON_COUNT_ADR + current_layer_);

        stage_ = Stage::ReadCurrentLayerNeuronsCountRead;

        cycles_to_wait = 1;
    }

    void stage_read_current_layer_neurons_count_read() {
        prev_layer_neurons_count_ = curr_layer_neurons_count_;
        curr_layer_neurons_count_ = net_config_rom_bus.data_read.data_i->read();

        weights_data_size_ = prev_layer_neurons_count_ * curr_layer_neurons_count_;

        for (auto& i : pe_bus) {
            i.compute_neuron_o->write(false);
            i.data_neuron_o->write(prev_layer_neurons_count_);
            i.rst_o->write(true);
        }

        readed_count_ = 0;

        net_config_rom_bus.data_read.rd_o->write(true);
        ram_bus.data_read.rd_o->write(true);

        stage_ = Stage::ReadLayerDataRequest;

        cycles_to_wait = 2;
    }

    void stage_read_layer_data_request() {
        for (auto& i : pe_bus) {
            i.rst_o->write(false);
        }

        net_config_rom_bus.adr_o.write(LAYERS_NEURON_COUNT_ADR + layers_count_ + readed_count_);
        ram_bus.adr_o->write(readed_count_);
        
        stage_ = Stage::ReadLayerDataRead;

        cycles_to_wait = 1;
    }

    void stage_read_layer_data_read() {
        auto neuron = ram_bus.data_read.data_i->read();
        auto weight = net_config_rom_bus.data_read.data_i->read();
    
        for (auto& pe : pe_bus) {
            pe.data_neuron_o->write(neuron);
            pe.data_weight_o->write(weight);
        }

        readed_count_++;

        if (readed_count_ >= weights_data_size_) {
            cycles_to_wait = 5;

            int a = 0;
            for (auto& i : pe_bus) {
                i.compute_neuron_o->write(true);
                i.rst_o->write(true);

                i.out_neuron_index_o->write(a);

                a++;
            }

            stage_ = Stage::Compute;
        } else {
            stage_ = Stage::ReadLayerDataRequest;
        }
    }


    void stage_compute() {
        for (auto& i : pe_bus) {
            i.rst_o->write(false);
        }

        for (auto& i : pe_bus) {
            if (i.is_done_i->read()) {
                auto computed_neuron = i.result_neuron_i->read();
                auto computed_neuron_index = i.out_neuron_index_o->read();

                std::cout << std::format(
                    "Neuorn {}, Result {}", 
                    computed_neuron_index,
                    *(float*)&computed_neuron
                ) << std::endl;

                ram_bus.adr_o->write(computed_neuron_index);
                ram_bus.data_write.wr_o->write(true);
                ram_bus.data_write.data_o->write(computed_neuron);

                computed_neurons_++;
                
                i.out_neuron_index_o->write(computed_neurons_);
                i.rst_o->write(true);
                

                cycles_to_wait = 1;

                if (computed_neurons_ >= curr_layer_neurons_count_) {
                    sc_core::sc_stop();
                }

                return;
            }


        }
    }

    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////

};

#endif // __OVS_ITMO_LAB_SRC_PE_DISPATCHER_H_