#ifndef __OVS_ITMO_LAB_SRC_PE_PE_H_
#define __OVS_ITMO_LAB_SRC_PE_PE_H_

#include "shared/ram.h"

#include "sysc/communication/sc_signal_ports.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_simcontext.h"
#include "systemc"
#include "tinyint.h"
#include <sys/_types/_pid_t.h>
#include <sys/wait.h>

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

        ComputeReq,
        Compute,
        ComputeFinish
    };

private:
    u32 previous_layer_neuron_count_ = 0;
    u32 previous_layer_neuron_index_ = 0;
    u32 data_adr_ = 0;
    
    float accumulator = 0;

    Stage stage_ = Stage::DataReadGetPreviousLayerNeuronCount;

    u32 cycles_to_wait_ = 0;

private:
    static constexpr u32 LAYERS_COUNT_ADR = 0x0000'0000;
    static constexpr u32 LAYERS_NEURON_COUNT_ADR = LAYERS_COUNT_ADR + 1;
    
private:
    void stage_data_read_get_previous_layer_neuron_count() {
        previous_layer_neuron_count_ = data_neurons_i->read();

        data_adr_ = 0;
        
        stage_ = Stage::DataReadWriteData;

        cycles_to_wait_ = 1;
    }

    void stage_data_read_write_data() {
        auto neuron_data = data_neurons_i->read();
        auto weight_data = data_weights_in_i->read();

        if (data_adr_ < previous_layer_neuron_count_) {
            layer_neurons_signals_.rd_.write(false);
            layer_neurons_signals_.wr_.write(true);
            layer_neurons_signals_.addr_.write(data_adr_);
            layer_neurons_signals_.data_bo_.write(neuron_data);
        }

        layer_weights_signals_.rd_.write(false);
        layer_weights_signals_.wr_.write(true);
        layer_weights_signals_.addr_.write(data_adr_);
        layer_weights_signals_.data_bo_.write(weight_data);

        data_adr_++;

        cycles_to_wait_ = 2;
    }

    void stage_compute_req() {
        is_computation_end_o->write(false);

        auto weight_index = (output_neuron_index_i->read() * previous_layer_neuron_count_) + previous_layer_neuron_index_;
        
        layer_neurons_signals_.wr_.write(false);
        layer_weights_signals_.wr_.write(false);
        layer_neurons_signals_.rd_.write(true);
        layer_weights_signals_.rd_.write(true);

        layer_neurons_signals_.addr_.write(previous_layer_neuron_index_);
        layer_weights_signals_.addr_.write(weight_index);

        cycles_to_wait_ = 3;

        stage_ = Stage::Compute;
    }

    void stage_compute_computations() {
        auto neuron = layer_neurons_signals_.data_bi_.read();
        auto weight = layer_weights_signals_.data_bi_.read();

        auto neuron_f = *(float*)(&neuron);
        auto weight_f = *(float*)(&weight);

        accumulator += neuron_f * weight_f;

        previous_layer_neuron_index_++;
        if (previous_layer_neuron_index_ >= previous_layer_neuron_count_) {
            stage_ = Stage::ComputeFinish;
        } else {
            stage_ = Stage::ComputeReq;
        }
    }

    float activate(float x) {
        return 1.f / (1.f + exp(-x));
    }

    void stage_compute_finish() {
        auto res = activate(accumulator);

        u32 res_u32 = *(u32*)&res;

        result_neuron_o->write(res_u32);

        is_computation_end_o->write(true);
    }

    void reset() {
        previous_layer_neuron_index_ = 0;

        if (compute_mode_i->read() ) {
            is_computation_end_o->write(false);
            accumulator = 0;
            stage_ = Stage::ComputeReq;
        } else {
            previous_layer_neuron_count_ = 0;
            
            stage_ = Stage::DataReadGetPreviousLayerNeuronCount;
        }

        data_adr_ = 0;
    }

    void cycle() {
        if (cycles_to_wait_ > 0) {
            cycles_to_wait_--;
            return;
        }

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
            break;

        case Stage::ComputeReq:
            stage_compute_req();
            break;

        case Stage::Compute:
            stage_compute_computations();
            break;

        case Stage::ComputeFinish:
            stage_compute_finish();
            break;
        }
    }
};

#endif // __OVS_ITMO_LAB_SRC_PE_PE_H_