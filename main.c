#include "gbvm.h"
#include "gbvm_assembler.h"
#include "univ_malloc.h"
#include "gbvm_tui.h"

void processFlag(const char* program, const char* flag, int* argc, char*** argv);
void __exec_sm(Vm* vm, WINDOW* win);
void __assemble_sasm(WINDOW* win);
void __disassemble_sm(WINDOW* win);

void inputHandler(Vm* vm, WINDOW* win, int* highlight);
const char* inputFile = NULL;
const char* outputFile = NULL;
char buffer[256];
int limit = -1;
int debug = 0;

int main(int argc, char** argv)
{
    static Vm vm = { 0 };
    loadStandardCallsIntoVm(&vm);
    vm.disp = enterTUIMode();

    const char* program = getNextCmdLineArg(&argc, &argv);

    while (argc > 0) {
        const char* flag = getNextCmdLineArg(&argc, &argv);
        processFlag(program, flag, &argc, &argv);
    }

    int ch;
    int highlight = 0;
    do {
        do {
            InputMenu(vm.disp.windows[INPUT], &highlight, &ch);
        } while (ch != '\n');

        inputHandler(&vm, vm.disp.windows[INPUT], &highlight);
        wgetch(vm.disp.windows[INPUT]);
        vm.cpu.registers.IP.as_u64 = 0;
        setFlag(HALT, &vm.cpu, 0);

    } while (ch);
    return 0;
}

void processFlag(const char* program, const char* flag, int* argc, char*** argv)
{

    switch (flag[1]) {
    case 'l':
        limit = atoi(getNextCmdLineArg(argc, argv));
        return;
    default:
        fprintf(stdout, "Usage: %s  [-l <limit>]\n", program);
        displayMsgWithExit("Unknown option provided.");
    }
}

void inputHandler(Vm* vm, WINDOW* win, int* highlight)
{
    wclear(win);
    refreshWindow(win, getNameForWindow(INPUT), 5, 5, 3);
    wmove(win, 2, 4);
    switch (*highlight) {
    case ASSEMBLE_EXEC_SASM:
        __assemble_sasm(win);
        wmove(win, 4, 4);
        inputFile = outputFile;
        __exec_sm(vm, win);
        break;

    case EXEC_SM:
        readFilePath(win, "Enter the name of the SM file : ", &inputFile);
        __exec_sm(vm, win);
        break;

    case ASSEMBLE_SASM:
        __assemble_sasm(win);
        break;

    case DISASSEMBLE_SM:
        __disassemble_sm(win);
        break;

    case COMPILE_ORIN:
    case EXIT_VM:
        exitTUIMode(&vm->disp);
    default:
        break;
    }
}

void __exec_sm(Vm* vm, WINDOW* win)
{
    loadProgramIntoVm(vm, inputFile);

    wprintw(win,
        "\n     Debug Mode?"
        "\n     0. No"
        "\n     1. Yes"
        "\n     2. Fast Debug"
        "\n     Your choice : ");
    refreshWindow(win, getNameForWindow(INPUT), 5, 5, 3);

    debug = wgetch(win) - '0';
    executeProgram(vm, debug, -1);
}

void __assemble_sasm(WINDOW* win)
{
    readFilePath(win, "Enter the name of the SASM file : ", &inputFile);
    wmove(win, 3, 4);
    readFilePath(win, "Enter the name of the output SM file : ", &outputFile);

    snprintf(buffer, sizeof(buffer), "./sasm -i %s -o %s", inputFile, outputFile);

    if (system(buffer) != 0) {
        displayMsgWithExit("Assembly Failed");
    }
}

void __disassemble_sm(WINDOW* win)
{
    readFilePath(win, "Enter the name of the SM file : ", &inputFile);
    wmove(win, 3, 4);
    readFilePath(win, "Enter the name of the output SASM file : ", &outputFile);

    snprintf(buffer, sizeof(buffer), "./sasm -i %s -o %s -d", inputFile, outputFile);

    if (system(buffer) != 0) {
        displayMsgWithExit("Disassembly Failed");
    }
}