#include "cpu.h"

using namespace std;

CPU::CPU(sc_module_name nm)
    :sc_module(nm),
    clk_i("clk_i"),
    addr_bo("addr_bo"),
    data_bi("data_bi"),
    data_bo("data_bo"),
    wr_o("wr_o"),
    rd_o("rd_o")
{
    addr_bo.initialize(0);
    data_bo.initialize(0);
    wr_o.initialize(0);
    rd_o.initialize(0);
    
    SC_CTHREAD(mainThread, clk_i.pos());
    
}

CPU::~CPU()
{

}

void CPU::mainThread()
{
    int data_size = 5;
    
    for(int i = 0; i < data_size; i++)
    {
        bus_write(i, (i+1)*2);
    }
    
    for(int i = 0; i < data_size; i++)
    {
        bus_read(i);
    }
    
    sc_stop();

}

int CPU::bus_read(int addr)
{
    int data;

    wait();
    addr_bo.write(addr);
    rd_o.write(1);
    
    wait();
    rd_o.write(0);
    
    wait();
    data = data_bi.read();
    
    cout << "CPU: READ " << endl;
    cout << "  -> addr: " << hex << addr << endl;
    cout << "  -> data: " << hex << data << endl;
    
    return data;
    
}

void CPU::bus_write(int addr, int data)
{
    wait();
    addr_bo.write(addr);
    data_bo.write(data);
    wr_o.write(1);
    
    wait();
    wr_o.write(0);
    
    cout << "CPU: WRITE " << endl;
    cout << "  -> addr: " << hex << addr << endl;
    cout << "  -> data: " << hex << data << endl;

}
