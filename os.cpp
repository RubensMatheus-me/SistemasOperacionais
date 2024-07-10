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
	
	
	if(interrupt == Arch::InterruptCode::Keyboard) {
		int typed_char = terminalTeste->read_typed_char();
		char char_typed = static_cast<char>(typed_char);
					
	}
}

// ---------------------------------------

void syscall ()
{

}

// ---------------------------------------

} // end namespace OS
