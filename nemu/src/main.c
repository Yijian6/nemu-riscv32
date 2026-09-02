void init_monitor(int, char *[]);
void reg_test();
void restart();
void ui_mainloop();

int main(int argc, char *argv[]) {

	/* Initialize the monitor. 1️⃣初始化monitor()*/
	init_monitor(argc, argv);

	/* Test the implementation of the `CPU_state' structure. 2️⃣检测CPU状态的结构的实施(测试register寄存器结构的实现)*/
	reg_test();

	/* Initialize the virtual computer system. */
	restart();

	/* Receive commands from user. */
	ui_mainloop();

	return 0;
}
