#ifndef __OVS_ITMO_LAB_SRC_PE_PE_H_
#define __OVS_ITMO_LAB_SRC_PE_PE_H_

#include "shared/ram.h"

#include "sysc/communication/sc_signal_ports.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_simcontext.h"
#include "systemc"
#include "tinyint.h"
#include <functional>
#include <sys/_types/_pid_t.h>
#include <sys/wait.h>

#include "shared/memory_bus.h"



SC_MODULE(PeCore) {
public:
    // Входы
    sc_core::sc_in<bool> compute_mode_i;  // 0 - режим чтения весов между слоями во внутренюю память
                                          // 1 - режим вычисления нейрона

    sc_core::sc_in<bool> clk_i;                  // Синхросигнал
    sc_core::sc_in<bool> rst_i;                  // Сброс

    sc_core::sc_in<u32>  output_neuron_index_i;  // Индекс вычисляемого выходного нейрона
    sc_core::sc_in<u32>  data_weights_in_i;      // Шина данных весов
    sc_core::sc_in<u32>  data_neurons_i;         // Шина данных нейронов

    // Выходы
    sc_core::sc_out<bool> is_computation_end_o;   // Флаг готовности к работе
    sc_core::sc_out<u32>  result_neuron_o;        // Посчитанный нейрон

private:
    struct InternalMemorySignals {
        sc_core::sc_signal<u32> addr_;
        sc_core::sc_signal<u32> data_bo_;
        sc_core::sc_signal<u32> data_bi_;
        sc_core::sc_signal<bool> wr_;
        sc_core::sc_signal<bool> rd_;
    };

    InternalMemorySignals layer_weights_signals_;
    InternalMemorySignals layer_neurons_signals_;
    
private:
    RandomAccessMemory layer_weights_ram_;
    RandomAccessMemory layer_neurons_ram_;
    
public: 
    PeCore(sc_core::sc_module_name name, size_t weights_ram_size, size_t neurons_ram_size) 
        : sc_core::sc_module(name)
        , layer_weights_ram_("layer_weights_ram_", weights_ram_size) 
        , layer_neurons_ram_("layer_neurons_ram_", neurons_ram_size)
    {
        std::pair<RandomAccessMemory&, InternalMemorySignals&> s[] = {
            { layer_weights_ram_, layer_weights_signals_ },
            { layer_neurons_ram_, layer_neurons_signals_ }
        };

        for (auto& i : s) {
            i.first.clk_i(clk_i);

            i.first.addr_bi(i.second.addr_);
            i.first.data_bi(i.second.data_bo_);
            i.first.wr_i(i.second.wr_);
            i.first.data_bo(i.second.data_bi_);
            i.first.rd_i(i.second.rd_);
        }

        is_computation_end_o.initialize(0);
        result_neuron_o.initialize(0);

        SC_METHOD(cycle);
        sensitive << clk_i.pos();
    }

private:
    enum class Stage {
        DataReadGetPreviousLayerNeuronCount,
        DataReadWriteData,

        Compute,
        ComputeFinish
    };

private:
    u32 previous_layer_neuron_count_ = 0;
    u32 previous_layer_neuron_index_ = 0;
    u32 data_adr_ = 0;
    float accumulator = 0;

    Stage stage_ = Stage::DataReadGetPreviousLayerNeuronCount;

private:
    static constexpr u32 LAYERS_COUNT_ADR = 0x0000'0000;
    static constexpr u32 LAYERS_NEURON_COUNT_ADR = LAYERS_COUNT_ADR + 1;
    
private:
    

    void stage_data_read_get_previous_layer_neuron_count() {
        previous_layer_neuron_count_ = data_neurons_i->read();

        data_adr_ = 0;
        
        stage_ = Stage::DataReadWriteData;
    }

    void stage_data_read_write_data() {
        auto neuron_data = data_neurons_i->read();
        auto weight_data = data_neurons_i->read();

        if (data_adr_ < previous_layer_neuron_count_) {
            memory_write(layer_neurons_signals_, data_adr_, neuron_data);
        }

        memory_write(layer_weights_signals_, data_adr_, weight_data);

        data_adr_++;
    }

    void stage_compute_computations() {
        auto weight_index = (output_neuron_index_i->read() * previous_layer_neuron_count_) + previous_layer_neuron_index_;
        
        auto neuron = memory_read(layer_neurons_signals_, previous_layer_neuron_index_);
        auto weight = memory_read(layer_weights_signals_, weight_index);

        auto neuron_f = *(float*)(&neuron);
        auto weight_f = *(float*)(&weight);

        accumulator += neuron_f * weight_f;

        previous_layer_neuron_index_++;
        if (previous_layer_neuron_index_ >= previous_layer_neuron_count_) {
            stage_ = Stage::ComputeFinish;
        }
    }

    float activate(float x) {
        return 1.f / (1.f + exp(-x));
    }

    void stage_compute_finish() {
        auto res = activate(accumulator);

        result_neuron_o->write(res);

        is_computation_end_o->write(true);
    }

    void reset() {
        accumulator = 0;
        previous_layer_neuron_count_ = 0;
        previous_layer_neuron_index_ = 0;
        data_adr_ = 0;

        is_computation_end_o->write(false);

        stage_ = compute_mode_i->read() ? Stage::Compute : Stage::DataReadGetPreviousLayerNeuronCount;
    }

    void cycle() {
        if (rst_i->read()) {
            reset();
            return;
        }

        switch (stage_) {
        case Stage::DataReadGetPreviousLayerNeuronCount:
            stage_data_read_get_previous_layer_neuron_count();
            break;

        case Stage::DataReadWriteData:
            stage_data_read_write_data();

        case Stage::Compute:
            stage_compute_computations();

        case Stage::ComputeFinish:
            stage_compute_finish();
        }
    }

    void main_thread() {
        // cycle();
        while (1) { cycle(); }
    }

    void memory_write(InternalMemorySignals& membus, u32 addr, u32 data) {
        // wait();
        membus.addr_.write(addr);
        membus.data_bo_.write(data);
        membus.wr_.write(1);

        // wait();
        membus.wr_.write(0);
    }

    u32 memory_read(InternalMemorySignals& membus, u32 addr) {
        u32 data;

        // wait();
        membus.addr_.write(addr);
        membus.rd_.write(1);

        // wait();
        membus.rd_.write(0);

        // wait();
        data = membus.data_bi_.read();

        return data;
    }
};

#endif // __OVS_ITMO_LAB_SRC_PE_PE_H_