#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"
#include "os.h"


namespace OS {

// ---------------------------------------
Arch::Terminal *terminalTeste;
Arch::Cpu *cpuTeste;

void boot (Arch::Terminal *terminal, Arch::Cpu *cpu)
{	
	terminalTeste = terminal;
	cpuTeste = cpu;
	terminal->println(Arch::Terminal::Type::Command, "Type commands here");
	terminal->println(Arch::Terminal::Type::App, "Apps output here");
	terminal->println(Arch::Terminal::Type::Kernel, "Kernel output here");
}

// ---------------------------------------

void interrupt (const Arch::InterruptCode interrupt){
	int typed_char = terminalTeste->read_typed_char();
	std::string x = "";
	std::cin >> x;
	if(interrupt == Arch::InterruptCode::Keyboard) {
		terminalTeste->println(Arch::Terminal::Type::Command, x);
		
		if (terminalTeste->is_backspace(typed_char)) {
			terminalTeste->println(Arch::Terminal::Type::Command, "apagar");
		}
		
		else if (terminalTeste->is_alpha(typed_char)) {	
			terminalTeste->println(Arch::Terminal::Type::Command, "letra");	
		}
		
		else if (terminalTeste->is_num(typed_char)) {
			terminalTeste->println(Arch::Terminal::Type::Command, "numero");
		}
		
		else if (terminalTeste->is_return(typed_char)) {
			terminalTeste->println(Arch::Terminal::Type::Command, "enter");
		}
		
		
	}
}

// ---------------------------------------

void syscall ()
{

}

// ---------------------------------------

} // end namespace OS
