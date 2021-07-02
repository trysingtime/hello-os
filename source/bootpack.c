void io_hlt(void); // 待机
void io_cli(void); // 中断标志置0, 禁止中断
void io_out8(int port, int data); //向指定设备(port)输出数据
int io_load_eflags(void); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
void io_store_eflags(int eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))

void init_palette(void); // 初始化调色盘
void set_palette(int start, int end, unsigned char *rgb); // 设置调色盘
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); // 绘制矩形
void init_screen(char *vram, int screenx, int screeny); // 初始化屏幕

// 定义色号和颜色映射关系
#define COL8_000000		0   /*  0:黑 */
#define COL8_FF0000		1   /*  1:梁红 */
#define COL8_00FF00		2   /*  2:亮绿 */
#define COL8_FFFF00		3   /*  3:亮黄 */
#define COL8_0000FF		4   /*  4:亮蓝 */
#define COL8_FF00FF		5   /*  5:亮紫 */
#define COL8_00FFFF		6   /*  6:浅亮蓝 */
#define COL8_FFFFFF		7   /*  7:白 */
#define COL8_C6C6C6		8   /*  8:亮灰 */
#define COL8_840000		9   /*  9:暗红 */
#define COL8_008400		10  /* 10:暗绿 */
#define COL8_848400		11  /* 11:暗黄 */
#define COL8_000084		12  /* 12:暗青 */
#define COL8_840084		13  /* 13:暗紫 */
#define COL8_008484		14  /* 14:浅暗蓝 */
#define COL8_848484		15  /* 15:暗灰 */

struct BOOTINFO {
    // 缓存在指定位置的BOOT_INFO(asmhead.nas中)
    char cyls, leds, vmode, reserve;
    short screenx, screeny;  // 分辨率
    char *vram; // VRAM起始地址
};

void HariMain(void) {
    struct BOOTINFO *bootinfo = (struct BOOTINFO *)0x0ff0;
    // 使用16字节定义一个8x16像素的字符"A"
    static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};

    init_palette(); // 设定调色盘
    init_screen(bootinfo -> vram, bootinfo -> screenx, bootinfo -> screeny); // 初始化屏幕
    putfont8(bootinfo -> vram, bootinfo -> screenx, 10, 10, COL8_FFFFFF, font_A); // 绘制字符

    // 待机
    for (;;) {
        io_hlt(); //执行naskfunc.nas里的_io_hlt
    }
}

void init_palette(void) {
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,	/*  0:黑 */
		0xff, 0x00, 0x00,	/*  1:梁红 */
		0x00, 0xff, 0x00,	/*  2:亮绿 */
		0xff, 0xff, 0x00,	/*  3:亮黄 */
		0x00, 0x00, 0xff,	/*  4:亮蓝 */
		0xff, 0x00, 0xff,	/*  5:亮紫 */
		0x00, 0xff, 0xff,	/*  6:浅亮蓝 */
		0xff, 0xff, 0xff,	/*  7:白 */
		0xc6, 0xc6, 0xc6,	/*  8:亮灰 */
		0x84, 0x00, 0x00,	/*  9:暗红 */
		0x00, 0x84, 0x00,	/* 10:暗绿 */
		0x84, 0x84, 0x00,	/* 11:暗黄 */
		0x00, 0x00, 0x84,	/* 12:暗青 */
		0x84, 0x00, 0x84,	/* 13:暗紫 */
		0x00, 0x84, 0x84,	/* 14:浅暗蓝 */
		0x84, 0x84, 0x84	/* 15:暗灰 */
    };
    set_palette(0, 15, table_rgb);
    return;
}

/*
    调色板访问步骤:
    1. 屏蔽中断(例如CLI)
    2. 设定调色板: 将调色板号写入0x03c8, 之后按照R,G,B顺序写入0x03c9三次
    3. 读取调色板: 将调色板号写入0x03c7, 之后按照R,G,B顺序读取0x03c9三次
    4. 恢复中断(例如STI)
*/
void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags(); // 读取EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
    io_cli(); // 中断标志置0, 禁止中断

    io_out8(0x03c8, start); // 调色板号写入0x03c8
    for (i = start; i <= end; i++) {
        // 按照R,G,B顺序写入0x03c9三次
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }

    io_store_eflags(eflags); // 还原EFLAGS寄存器(包含进位标志(第0位),中断标志(第9位))
    return;
}

/*
    绘制矩形
    *vram: vram起始地址
    screenx: 分辨率x轴大小
    color: 色号
    x0, y0, x1, y1: 矩形位置
*/
void boxfill8(unsigned char *vram, int screenx, unsigned char color, int x0, int y0, int x1, int y1) {
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x<= x1; x++) {
            // 像素坐标 = vram(0xa0000) + x + y * xsize(320)
            vram[x + y * screenx] = color;
        }
    }
    return;
}

/*
    初始化桌面
    *vram: vram起始地址
    screenx: 分辨率x轴大小
    screeny: 分辨率y轴大小
*/
void init_screen(char *vram, int screenx, int screeny) {
    // 绘制多个矩形
	boxfill8(vram, screenx, COL8_008484,  0,                    0, screenx -  1, screeny - 29); // 桌面背景色-浅暗蓝
	boxfill8(vram, screenx, COL8_C6C6C6,  0,         screeny - 28, screenx -  1, screeny - 28); // 过渡-灰白
	boxfill8(vram, screenx, COL8_FFFFFF,  0,         screeny - 27, screenx -  1, screeny - 27); // 过渡-白
	boxfill8(vram, screenx, COL8_C6C6C6,  0,         screeny - 26, screenx -  1, screeny -  1); // 任务栏背景色-亮灰

	boxfill8(vram, screenx, COL8_FFFFFF,  3,         screeny - 24, 59,         screeny - 24); // 开始按钮上边框-白
	boxfill8(vram, screenx, COL8_FFFFFF,  2,         screeny - 24,  2,         screeny -  4); // 开始按钮左边框-白
	boxfill8(vram, screenx, COL8_848484,  3,         screeny -  4, 59,         screeny -  4); // 开始按钮底边阴影-暗灰
	boxfill8(vram, screenx, COL8_848484, 59,         screeny - 23, 59,         screeny -  5); // 开始按钮右边阴影-暗灰
	boxfill8(vram, screenx, COL8_000000,  2,         screeny -  3, 59,         screeny -  3); // 开始按钮底边框-黑
	boxfill8(vram, screenx, COL8_000000, 60,         screeny - 24, 60,         screeny -  3); // 开始按钮右边框-黑

	boxfill8(vram, screenx, COL8_848484, screenx - 47, screeny - 24, screenx -  4, screeny - 24); // 任务状态栏凹槽上边框-暗灰
	boxfill8(vram, screenx, COL8_848484, screenx - 47, screeny - 23, screenx - 47, screeny -  4); // 任务状态栏凹槽左边框-暗灰
	boxfill8(vram, screenx, COL8_FFFFFF, screenx - 47, screeny -  3, screenx -  4, screeny -  3); // 任务状态栏凹槽底边框-白
	boxfill8(vram, screenx, COL8_FFFFFF, screenx -  3, screeny - 24, screenx -  3, screeny -  3); // 任务状态栏凹槽右边框-白
}

/*
    绘制字符
    *vram: vram起始地址
    screenx: 分辨率x轴大小
    x, y: 字符位置
    color: 色号
    *font: 字体数据(使用16字节定义一个8x16像素的字符)
*/
void putfont8(char *vram, int screenx, int x, int y, char color, char *font) {
    int i;
    char *p;
    for (i = 0; i <= 16; i++) {
        // 8x16像素字符, 每一行起始vram地址
        p = vram + (y + i) * screenx + x;
        if ((font[i] & 0x80) != 0) { p[0] = color; }
        if ((font[i] & 0x40) != 0) { p[1] = color; }
		if ((font[i] & 0x20) != 0) { p[2] = color; }
		if ((font[i] & 0x10) != 0) { p[3] = color; }
		if ((font[i] & 0x08) != 0) { p[4] = color; }
		if ((font[i] & 0x04) != 0) { p[5] = color; }
		if ((font[i] & 0x02) != 0) { p[6] = color; }
		if ((font[i] & 0x01) != 0) { p[7] = color; }
    }
}
