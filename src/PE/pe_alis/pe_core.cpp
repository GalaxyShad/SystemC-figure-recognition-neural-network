#include "pe_core.h"

PeCore::PeCore(sc_module_name nm)
    : sc_module(nm)
{	
	for (int i=7; i>=0; --i){
		int size = std::pow(2, i+1);
		int* array = new int[size];
		sum[i] = array;
	}
	
	SC_СTHREAD(matrix_mutliply);
	sensitive << is_muliply_ready.pos();
	
	SC_СTHREAD(add_tree);
	sensitive << is_sum_ready.pos();
	
	SC_СTHREAD(activaton_function);
	sensitive << is_activation_ready.pos();
	
	SC_СTHREAD(read_data);
	sensitive << clk_i.pos();
	
	SC_СTHREAD(update_read);
	sensitive << clk_i.pos();
	
	SC_СTHREAD(write_data);
	sensitive << clk_i.pos();
}

PeCore::~PeCore(){
	for (int i=0; i<8; ++i){
		delete[] sum[i];
	}
}

void PeCore::matrix_mutliply(){
	//ожидаем is_muliply_ready
	wait();
	
	// перемножение векторов
	for (int i=0; i<256; ++i){
		maxtix_mul_res[i] = maxrix_mul_first[i]*maxrix_mul_second[i];
	}
	is_muliply_ready.write(0);
	is_sum_ready.write(1);

}
 
 

void PeCore::add_tree(){
	//ожидаем is_sum_ready
	wait();
	
	//перенос данный с выхода мультиплекора на дерево сложения
	sum[7] = maxtix_mul_res;
	
	//сложение по дереву
	for (int lvl=7; lvl>0; --lvl){
		int size = std::pow(2, lvl);
		for (int i=0; i<size; ++i){
			sum[lvl-1][i] =  sum[lvl][i*2] + sum[lvl][i*2+1]; 
		}
	}
	is_sum_ready.write(0);
}

void PeCore::activaton_function(){
	// ожидаем is_activation_ready
	wait();
	
	//Записываем в выходной регистр 0
	ready_o.write(0);
	//перенос
	float x = sum[];  
	
	//функция активации
	neuron_out = 1/(1+exp(-x)); 
	ready_o.write(1);

}


//clk_i.pos
void PeCore::read_data(){
	wait();
	ready_o.write(1);
	if (read_signal_i.pos){
		wait();
		if ((mode == 0) && (data_counter != 256)){
			maxrix_mul_first[data_counter] = data_io.read();
		} else if (mode == 0){
			addr = data_io.read();
		} else {
			maxrix_mul_second[data_counter] = data_io.read();
		}
		++data_counter;
	}
}

//clk.pos
void PeCore::update_read(){
	wait();
	if (read_signal_i.neg){
		if (mode = 1){
			is_muliply_ready.write(1);
		}
		mode = (mode+1)/2;
		data_counter = 0;
	}
}

//clk.pos
void PeCore::write_data(){
	wait();
	if(ready_o.pos){
		wait();
		data_io.write(addr);
		data_io.write(neuron_out)
		
		wait();
		ready_o.write(0);
	}
}