#ifndef MYSYSTEMCPROJECT_PE_CORE_H
#define MYSYSTEMCPROJECT_PE_CORE_H

#include "systemc.h"

using AdrSize = uint32_t; // Размер шины адреса

// IO RAM
// [1] 1
// [2] 0
// [49] 1

// [1] 1
// [2] 0
// [49] 1

// in io_ram_read
// in weights_read
// out ready
// in data_i[257]

SC_MODULE(PeCore) {
	
	// ins
	sc_in<bool> read_signal_i;
	sc_in<bool> clk_i;
	sc_inout<float> data_io;

	//outs
	sc_out<bool> ready_o;
	sc_out<float> data_o;

	//constructor
	PeCore(sc_module_name name);
	~PeCore();

	//main process
	SC_HAS_PROCESS(PeCore);
	void main_thread();
	
	// events ???
	//sc_event process_event;


private:	
	// local registers
	sc_signal<bool> is_muliply_ready;
	sc_signal<bool> is_sum_ready;
	sc_signal<bool> is_activation_ready;
	sc_signal<bool> is_ready_to_write;
	
	std::vector<float> addr;
	std::vector<void*> sum[8];
	
	
	float maxrix_mul_first[256];
	float maxrix_mul_second[256];
	float maxtix_mul_res[256];

	float summ; 
	float neuron_out;
	
	int data_counter = 0; //0 -> 257 -> 0 -> 256 -> ... 
	int mode = 0; // 0 - читать IO RAM; 1 - читать RAM

	//subfunctions
	//void write_to_local_memory(AdrSize addr,float data);
	void matrix_mutliply();
	void activaton_function();
	void add_tree();
	
	void read_data();
	void update_read();
	void write_data();
};
/*
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

// Пока что конфигурация сети ROM
// TODO изменить на RAM и загружать данные через IO Controller
SC_MODULE(NetConfigRom) {
  sc_in<bool> clk_i;
  sc_in<AdrSize> addr_bi;
  sc_out<uint32_t> data_bo;
  sc_in<bool> rd_i;

  // TODO pass NeuralNetwork from repo to constructor
};

SC_MODULE(FigureRecognitionMicrocontroller) {
  sc_in<bool> clk_i;

  FigureRecognitionMicrocontroller(
      size_t cores_count // кол-во ядер 3..7

      );
};*/

#endif // MYSYSTEMCPROJECT_PE_CORE_H
