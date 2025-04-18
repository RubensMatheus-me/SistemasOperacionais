#include <stdexcept>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"
#include "os.h"


#define PAGE_SIZE 4096 //4KB

struct Process {
    uint16_t pc;
    size_t pid;
    bool active;
    std::array<uint16_t, 8> registers; 
    std::vector<uint16_t> page_table;
    uint16_t sleep_time;
};

namespace OS {

// ---------------------------------------
void load_program(const std::string_view binary_file);
Arch::Terminal *terminalSystem;
Arch::Cpu *cpuSystem;

uint16_t max_size = 65535;
uint32_t uptime_seconds = 0;


static std::string bufferedChar;

std::vector<Process> process_table;  

uint16_t get_process_limit(const std::string_view fname) {
    uint32_t file_size_words = Lib::get_file_size_words(fname);
    
    if (file_size_words > max_size) { 
        throw std::runtime_error("O arquivo excedeu o limite máximo da memória do processo.");
    }
    
    return static_cast<uint16_t>(file_size_words);
}

void reset_cpu_state() {
    for (uint16_t i = 0; i < Config::nregs; i++) {
        cpuSystem->set_gpr(i, 0);
    }

    cpuSystem->set_vmem_paddr_init(0);
    cpuSystem->set_vmem_paddr_end(-1);

    cpuSystem->set_pc(0);
}

void sleep_process(uint16_t seconds) {
    Process& proc = process_table.back();
    proc.sleep_time = seconds;
    terminalSystem->println(Arch::Terminal::Type::Kernel, "Processo vai dormir por ", seconds, " segundos.");

    proc.active = false;
}

uint32_t get_uptime() {
    return uptime_seconds;
}

uint16_t allocate_physical_frame() {
    static uint16_t next_frame = 0;
    static const uint16_t MAX_FRAMES = 1024;

    if (next_frame >= MAX_FRAMES) {
        throw std::runtime_error("Número máximo de frames físicos alcançado.");
    }

    return next_frame++;
}


void allocate_pages(uint16_t required_limit, Process& proc) {
    uint16_t num_pages = (required_limit + PAGE_SIZE - 1) / PAGE_SIZE;

    terminalSystem->println(Arch::Terminal::Type::Kernel, "Tamanho do binário: ", required_limit);
    terminalSystem->println(Arch::Terminal::Type::Kernel, "Número de páginas necessárias: ", num_pages);

    for (uint16_t i = 0; i < num_pages; ++i) {
        uint16_t physical_page = allocate_physical_frame();
        proc.page_table.push_back(physical_page);  
    }
}

void check_sleep() {
    uint32_t current_time = get_uptime();
    for (auto& proc : process_table) {
        if (proc.active == false && proc.sleep_time > 0) {
            if (current_time >= proc.sleep_time) {
                proc.sleep_time = 0; 
                proc.active = true;   
                terminalSystem->println(Arch::Terminal::Type::Kernel, "Processo acordado.");
            }
        }
    }
}

void vmem_write(uint16_t addr, uint16_t value, const Process& proc) {
    uint16_t page_num = addr / PAGE_SIZE;
    uint16_t offset = addr % PAGE_SIZE;

    if (page_num >= proc.page_table.size()) {
        terminalSystem->println(Arch::Terminal::Type::Kernel, "Erro: Acesso à página não mapeada.");
        return;
    }

    uint16_t physical_page_base = proc.page_table[page_num];
    uint16_t physical_addr = physical_page_base * PAGE_SIZE + offset;

    cpuSystem->pmem_write(physical_addr, value);
}

uint16_t vmem_read(uint16_t addr, const Process& proc) {
    uint16_t page_num = addr / PAGE_SIZE;
    uint16_t offset = addr % PAGE_SIZE;

    if (page_num >= proc.page_table.size()) {
        terminalSystem->println(Arch::Terminal::Type::Kernel, "Erro: Acesso à página não mapeada.");
        return 0;
    }

    uint16_t physical_page_base = proc.page_table[page_num];
    uint16_t physical_addr = physical_page_base * PAGE_SIZE + offset;

    return cpuSystem->pmem_read(physical_addr);
}


void kill_process(Process& proc) {
    //if (!proc.active) return; 

    terminalSystem->println(Arch::Terminal::Type::Kernel, "Finalizando o processo ativo");

    proc.page_table.clear();  
    proc.active = false;   
    reset_cpu_state();       
}

void load_program(const std::string_view binary_file) {
    reset_cpu_state();
    
    if (!process_table.empty() && process_table.back().active) {
        terminalSystem->println(Arch::Terminal::Type::Kernel, "Finalizando o processo ativo antes de carregar um novo.");
        kill_process(process_table.back());
    }

    Process new_process;
    std::fill(new_process.registers.begin(), new_process.registers.end(), 0);
    std::vector<uint16_t> loadBinary = Lib::load_from_disk_to_16bit_buffer(binary_file);

    uint16_t required_limit = loadBinary.size();
    allocate_pages(required_limit, new_process);

    for (uint16_t i = 0; i < loadBinary.size(); ++i) {
        uint16_t page_num = i / PAGE_SIZE;
        uint16_t offset = i % PAGE_SIZE;
        uint16_t physical_page = new_process.page_table[page_num];
        uint16_t physical_addr = physical_page * PAGE_SIZE + offset;

        cpuSystem->pmem_write(physical_addr, loadBinary[i]);
    }

    new_process.pc = new_process.page_table.front() * PAGE_SIZE;

    process_table.push_back(new_process);

    cpuSystem->set_vmem_paddr_init(new_process.page_table.front() * PAGE_SIZE);
    cpuSystem->set_vmem_paddr_end(new_process.page_table.back() * PAGE_SIZE + PAGE_SIZE - 1);
    }

void boot(Arch::Terminal *terminal, Arch::Cpu *cpu) {
    terminalSystem = terminal;
    cpuSystem = cpu;
    
    const std::string_view binary_file = "idle.bin";
    load_program(binary_file);
    terminalSystem->println(Arch::Terminal::Type::Kernel, "idle.bin em processo...");
    
    terminal->println(Arch::Terminal::Type::Command, "Type commands here");
    terminal->println(Arch::Terminal::Type::App, "Apps output here");
    terminal->println(Arch::Terminal::Type::Kernel, "Kernel output here");
}


void info_process() {
    terminalSystem->println(Arch::Terminal::Type::Kernel, "Informações do processo:");

    if (!process_table.empty()) {
        const Process& proc = process_table.back();
        
        terminalSystem->println(Arch::Terminal::Type::Kernel, "Número de páginas: ", proc.page_table.size());

        terminalSystem->println(Arch::Terminal::Type::Kernel, "PC (Contador de Programa): ", proc.pc);

        terminalSystem->println(Arch::Terminal::Type::Kernel, "Páginas alocadas: ");
        for (const auto& page : proc.page_table) {
            terminalSystem->println(Arch::Terminal::Type::Kernel, "Página: ", page);
        }

    } else {
        terminalSystem->println(Arch::Terminal::Type::Kernel, "Nenhum processo em execução.");
    }

}

void interrupt(const Arch::InterruptCode interrupt) {
		
    if (interrupt == Arch::InterruptCode::GPF) {
        terminalSystem->println(Arch::Terminal::Type::Kernel, "Falha geral de proteção (GPF). Finalizando o processo.");

        if (!process_table.empty() && process_table.back().active) {
            kill_process(process_table.back());
        }

        if (process_table.empty()) {
            load_program("idle.bin");
            terminalSystem->println(Arch::Terminal::Type::Kernel, "Processo idle.bin recarregado.");
        }
        return;
    }
    if (interrupt == Arch::InterruptCode::Timer) {  
        if (!process_table.empty() && process_table.back().sleep_time > 0) {
            Process& proc = process_table.back();
            proc.sleep_time--; 

            if (proc.sleep_time == 0) {
                proc.active = true;  
            }
        }
   }
    if (interrupt == Arch::InterruptCode::Keyboard) {
        int typed_char = terminalSystem->read_typed_char();
        char char_typed = static_cast<char>(typed_char);

        if (terminalSystem->is_backspace(typed_char)) {
            if (!bufferedChar.empty()) {
                bufferedChar.pop_back();
                for (size_t i = 0; i < bufferedChar.size() + 1; ++i) {
                    terminalSystem->print(Arch::Terminal::Type::Command, '\r'); 
                    terminalSystem->print(Arch::Terminal::Type::Command, bufferedChar);
                }
            }
            terminalSystem->println(Arch::Terminal::Type::Kernel, "apagar");
        }

        else if (terminalSystem->is_alpha(typed_char) || terminalSystem->is_num(typed_char)) {
            bufferedChar += char_typed;
            terminalSystem->print(Arch::Terminal::Type::Command, char_typed);
        }

        else if (terminalSystem->is_return(typed_char)) {
            terminalSystem->print(Arch::Terminal::Type::Command, '\n');
            terminalSystem->println(Arch::Terminal::Type::Kernel, "enter");
            terminalSystem->print(Arch::Terminal::Type::App, bufferedChar);
            terminalSystem->println(Arch::Terminal::Type::App, "");
            
            if (bufferedChar.rfind("load", 0) == 0) { 
                std::string_view filename = bufferedChar.substr(4) + ".bin";
                load_program(filename);
            } else if (bufferedChar == "killprocess") {

			if (!process_table.empty()) {
				terminalSystem->println(Arch::Terminal::Type::Kernel, "Comando killprocess recebido");
				terminalSystem->println(Arch::Terminal::Type::Kernel, "Número de processos: ", process_table.size());
				kill_process(process_table.back());
			} else {
				terminalSystem->println(Arch::Terminal::Type::Kernel, "Nenhum processo ativo para matar.");
			}

            } else if (bufferedChar == "infoprocess") {
                info_process();
            } else if (bufferedChar == "quit") {
                cpuSystem->Cpu::turn_off();
            }
            
            bufferedChar.clear(); 
        }
       
    }
}

// ---------------------------------------

void syscall() {
    uint16_t r0 = cpuSystem->get_gpr(0);

    switch (r0) {
        case 0: 
            cpuSystem->Cpu::turn_off();
            break;
        case 1: { 
            uint16_t addr = cpuSystem->get_gpr(1);
            std::string str;
            char c;
            while ((c = static_cast<char>(vmem_read(addr, process_table.back()))) != '\0') {
                str += c;
                addr++;
            }
            terminalSystem->print(Arch::Terminal::Type::App, str);
            break;
        }
        case 2: 
            terminalSystem->println(Arch::Terminal::Type::App, "");
            break;
        case 3: {
            int number = cpuSystem->get_gpr(1);
            number += 1;
            cpuSystem->set_gpr(1, number);
            terminalSystem->print(Arch::Terminal::Type::App, number);
            break;
        }
        case 4: {
            if (!process_table.empty()) {
                kill_process(process_table.back());
            }
            break;
        }
        default:
            terminalSystem->println(Arch::Terminal::Type::Kernel, "Unknown syscall");
            break;
    }
}


// ---------------------------------------

} // end namespace OS
