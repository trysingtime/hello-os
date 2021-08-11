/* asmhead.nas */

// 缓存在指定位置的BOOT_INFO(asmhead.nas中)
struct BOOTINFO {
    char cyls;              // 启动时读取的柱面数
    char leds;              // 启动时键盘LED的状态
    char vmode;             // 显卡模式为多少位彩色
    char reserve;
    short screenx, screeny; // 画面分辨率
    char *vram; // VRAM起始地址
};
#define ADR_BOOTINFO    0x00000ff0

/* naskfunc.nas */
/* 待机 */
void io_hlt(void);
/* 中断标志置0, 禁止中断 */
void io_cli(void);
/* 中断标志置1, 允许中断 */
void io_sti(void);
/* 允许中断并待机 */
void io_stihlt(void);
/* 从指定端口读取一个字节 */
int io_in8(int port);
/* 向指定设备(port)输出数据 */
void io_out8(int port, int data);
/* 读取EFLAGS寄存器(包含进位标志CF(第0位),中断标志IF(第9位),AC标志位(第18位, 486CPU以上才有)) */
int io_load_eflags(void);
/* 还原EFLAGS寄存器(包含进位标志CF(第0位),中断标志IF(第9位),AC标志位(第18位, 486CPU以上才有)) */
void io_store_eflags(int eflags);
/* 从addr指定的地址读取一个字节 */
char read_mem8(int addr);
/* 把已知的GDT起始地址和段个数加载到GDTR寄存器 */
void load_gdtr(int limit, int addr);
/* 把已知的IDT起始地址和中断个数加载到IDTR寄存器 */
void load_idtr(int limit, int addr);
/* 向TR(task register)寄存器(任务切换时值会自动变化)存入数值 */
void load_tr(int tr);
/* CR0寄存器(32位)读取值,bit30+bit29置1禁止缓存,bit31置为0禁用分页,bit0置为1切换到保护模式 */
int load_cr0(void);
/* CR0寄存器(32位)存储值 */
void store_cr0(int cr0);
/* 定时器中断处理函数 */
void asm_inthandler20(void);
/* 键盘中断处理函数 */
void asm_inthandler21(void);
/* 电气噪声处理函数 */
void asm_inthandler27(void);
/* 鼠标中断处理函数 */
void asm_inthandler2c(void);
/* 内存容量检查 */
unsigned int memtest_sub(unsigned int start, unsigned int end);
/* 
    far跳转, 目的地址为cs:eip, 若目的地址为TSS, 则为任务切换
    - cs:eip: 目的地址, 若cs为TSS段号, 则eip没有作用, 一般设置为0
*/
void farjmp(int eip, int cs);

/* fifo.c */

// 缓冲区结构
struct FIFO32 {
    int *buf; // 缓存区地址
    int p, q, size, free, flags; // 写入位置, 读出位置, 缓存区总大小, 空余大小, 溢出标识
    struct TASK *task; // 满足条件后自动唤醒task
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task); // 初始化缓冲区
int fifo32_put(struct FIFO32 *fifo, int data); // 缓冲区写入1字节
int fifo32_get(struct FIFO32 *fifo); // 缓冲区读出1字节
int fifo32_status(struct FIFO32 *fifo); // 缓冲区当前深度

/* graphic.c */

void init_palette(void); // 初始化调色盘
void set_palette(int start, int end, unsigned char *rgb); // 设置调色盘
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); // 绘制矩形
void init_screen8(char *vram, int screenx, int screeny); // 初始化屏幕
void putfont8(char *vram, int screenx, int x, int y, char color, char *font); // 绘制字符
void putfonts8_asc(char *vram, int screenx, int x, int y, char color, unsigned char *str); // 绘制字符串
void init_mouse_cursor8(char *mouse, char bc); // 初始化鼠标指针(16x16像素)像素点颜色数据
void putblock8_8(char *vram, int screenx, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize); // 绘制图形

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

/* dsctbl.c */

/*
SEGMENT_DESCRIPTOR(8字节, GDT单元结构)保存段起始地址,上限地址及段属性
- base(32位):该段起始地址
- limit(20位, limit_high上4位被用于access_right):该段地址上限值
- access_right(段的属性): 12位, limit_high高4位被用于access_right的高4位, 一般使用16位表示(xxxx0000xxxxxxxx)
    -- 高4位被称为扩展访问器, 一般为"GD00"
        G表示Gbit(granularity粒度), 为0时表示limit单位为byte, 上限为1MB; 为1时表示limit单位为page(4KB), 上限为4BK*1MB=4G
        D表示段的模式, 0是16位模式, 用于80286CPU, 不能调用BIOS; 1是32位模式, 除80286外一般D=1
    -- 低8位
        00000000(0x00): 未使用的记录表
        10010010(0x92): 系统专用, 可读写的段. 不可执行
        10011010(0x9a): 系统专用, 可执行的段. 可读不可写
        11110010(0xf2): 应用程序用, 可读写的段. 不可指定
        11111010(0xfa): 应用程序用, 可执行的段. 可读不可写
*/
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

/*
    GATE_DESCRIPTOR(8字节), 存储中断函数地址, 段号, 属性
    - offset: 中断函数地址(传入函数名即传入函数首地址)
    - selector: 段号
    - access_right: 属性
*/
struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

void init_gdtidt(void); // 设定GDT/IDT的起始地址和上限地址, 并初始化GDT/IDT(调用set_segmdesc/set_gatedesc), 定义每个段号对应的段信息/每个中断号对应的函数信息
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int access_right); // 设置每个段号对应的段信息
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int access_right); // 设置每个中断号对应的函数信息
#define ADR_IDT			0x0026f800 // IDT起始地址
#define LIMIT_IDT		0x000007ff // IDT上限地址
#define ADR_GDT			0x00270000 // GDT起始地址
#define LIMIT_GDT		0x0000ffff // GDT上限地址
#define ADR_BOTPAK		0x00280000 // 段号2起始地址
#define LIMIT_BOTPAK	0x0007ffff // 段号上限地址
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */

void init_pic(void); // 初始化PIC
void inthandler27(int *esp); // 电气噪声处理函数
#define PIC0_IMR		0x0021  // IMR(interrupt mask register)地址: PIC的8位寄存器
/* 
    ICW(initial control word): 有4个(ICW1-ICW4)
    - ICW1和ICW4配置与PIC主板的配线方式, 根据硬件已固定
    - ICW3(8位)每位置为1对应一个从PIC, 根据硬件已固定
    - ICW2(8位)决定IRQ触发时哪一个中断信号(例如INT 0x20), CPU根据IDT设置调用中断处理函数(需自己配置IDT和编写该处理函数)
*/ 
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_IMR		0x00a1  // IMR(interrupt mask register)地址: PIC1的8位寄存器
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */

void inthandler21(int *esp); // 键盘中断处理函数
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int offsetdata);
#define PORT_KEYDAT             0x0060      /* 数据端口(键盘/鼠标/A20GATE信号线) */
#define PORT_KEYCMD             0x0064      /* 键盘控制器端口(用于设置) */

/* mouse.c */

/*
    鼠标数据结构体
*/
struct MOUSE_DEC {
    unsigned char buf[3], phase; // 缓冲鼠标数据, 鼠标阶段
    int x, y, btn; // 鼠标x轴, y轴, 按键
};
void inthandler2c(int *esp); // 鼠标中断处理函数
void enable_mouse(struct FIFO32 *fifo, int offsetdata, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

/* memory.c */

#define MEMMNG_ADDR     0x003c0000; // 内存管理表起始地址
#define MEMMNG_SIZE     4096 // 空闲内存信息总数: 使用4096个FREEINFO结构记录空闲内存信息
/*
    内存空闲信息结构体
    使用8字节记录某一段空闲内存地址起点和大小
*/
struct FREEINFO {
    unsigned int addr, size;
};
/*
    内存控制器
*/
struct MEMMNG {
    int rows;        // 内存空闲信息条数
    int maxrows;     // row最大值
    int lostsize;   // 内存空闲信息条数溢出, 导致内存释放失败的内存大小总和
    int lostrows;    // 内存空闲信息条数溢出, 导致内存释放失败次数
    struct FREEINFO freeinfo[MEMMNG_SIZE]; // 内存空闲信息, 使用4096个FREEINFO结构记录空闲内存
};
unsigned int memtest(unsigned int start, unsigned int end);
void memmng_init(struct MEMMNG  *mng);
unsigned int free_memory_total(struct MEMMNG *mng);
unsigned int memory_alloc(struct MEMMNG *mng, unsigned int size);
int memory_free(struct MEMMNG *mng, unsigned int addr, unsigned int size);
unsigned int memory_alloc_4k(struct MEMMNG *mng, unsigned int size);
int memory_free_4k(struct MEMMNG *mng, unsigned int addr, unsigned int size);

/* layer.c */

#define MAX_LAYERS      256 // 最大图层数
/*
    图层
    buf: 图层关联的内容地址;
    bxsize, bysize: 图层大小;
    vx0, v0: 图层坐标
    col_inv: color(颜色)和invisible(透明度)
    height: 图层高度
    flags: 图层已使用标识
*/
struct LAYER {
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    struct LAYERCTL *ctl;
};
/*
    图层控制器
    - vram, xsiez, ysize: vram地址和画面大小, 不用每次去获取BOOTINFO中的启动信息
    - top: 最顶层图层高度
    - layersorted: 图层根据高度升序排序索引
    - layer: 图层
*/
struct LAYERCTL {
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct LAYER *layersorted[MAX_LAYERS];
    struct LAYER layers[MAX_LAYERS];
};
struct LAYERCTL *layerctl_init(struct MEMMNG *memmng, unsigned char *vram, int xsize, int ysize);
struct LAYER *layer_alloc(struct LAYERCTL * ctl);
void layer_init(struct LAYER *layer, unsigned char *buf, int xsize, int ysize, int col_inv);
void layer_refresh(struct LAYER *layer, int bx0, int by0, int bx1, int by1);
void layer_refresh_abs(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void layer_refresh_map(struct LAYERCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void layer_updown( struct LAYER *layer, int height);
void layer_slide(struct LAYER *layer, int vx0, int vy0);
void layer_free(struct LAYER *layer);

/* timer.c */

#define MAX_TIMER       500 // 定时器上限数
/*
    定时器
    - next: 下一个即将超时的定时器(链表结构)
    - timeout: 倒计时
    - flags: 定时器状态标识
    - fifo, data: 倒计时结束后往fifo缓冲区发送数据data
*/
struct TIMER {
    struct TIMER *next;
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    int data;
};
/*
    定时控制器
    - count: 计时(每秒100)
    - nexttime: 下一个触发时刻(单位同count)
    - nextnode: 下一个触发节点
    - timer: 定时器
*/
struct TIMERCTL {
    unsigned int count, nexttime;
    struct TIMER *nextnode;
    struct TIMER timer[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);

/* multitask.c */

#define MAX_TASKS       1000 // 最大任务数量
#define MAX_TASK_LEVELS 10 // 最大任务层级
#define TASK_GDT0       3 // GDT从几号开始分配给TSS
/*
    TSS(task status segment)(32位, 26个int成员, 总计104字节): 任务状态段, 需要在GDT注册才能使用
    CPU任务切换时, 将寄存器的值保存在TSS中, 后续切换回任务时, 再读取回去
    如果一条JMP指令的目的地址段是TSS, 则解析为任务切换
*/
struct TSS32 {
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3; // 任务设置相关的信息(任务切换时不变)
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; // 任务切换时保存32位寄存器值
	int es, cs, ss, ds, fs, gs; // 任务切换时保存16位寄存器值
	int ldtr, iomap; // 任务设置相关的信息(任务切换时不变)
};
/*
    任务
    - selector: GDT中TSS编号
    - flags: 任务状态, 0: 未激活,1: 正在使用,2: 正在运行
    - level: 当前任务属于哪个层级(level)
    - priority: 任务优先级(任务切换间隔, 执行多少秒后切换到下一个任务, 单位: priority/100s)
    - tss: TSS结构
*/
struct TASK {
    int selector, flags;
    int level, priority;
    struct TSS32 tss;
};
/*
    任务层级
    - running_number: 当前层级正在运行的任务数量
    - current: 当前层级当前运行的是哪个任务
    - index: 当前层级正在运行任务的升序索引
*/
struct TASKLEVEL {
    int running_number;
    int current;
    struct TASK *index[MAX_TASKS / MAX_TASK_LEVELS];

};
/*
    任务控制器
    - current_level: 正在活动的任务层级
    - level_change: 任务层级变动标识, 若为1则表明层级有变动, 需要更新层级信息
    - level: 所有任务层级
    - tasks: 所有任务
*/
struct TASKCTL {
    int current_level;
    char level_change;
    struct TASKLEVEL level[MAX_TASK_LEVELS];
    struct TASK tasks[MAX_TASKS];
};
extern struct TIMER *task_timer;
struct TASK *taskctl_init(struct MEMMNG *memmng);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
