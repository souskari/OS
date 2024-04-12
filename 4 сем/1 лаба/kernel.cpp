// Эта инструкция обязательно должна быть первой, т.к. этот код компилируется в бинарный,
// и загрузчик передает управление по адресу первой инструкции бинарногообраза ядра ОС.
__asm("jmp kmain");

#define VIDEO_BUF_PTR (0xb8000)
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
#define GDT_CS (0x8)
#define PIC1_PORT (0x20)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80)
unsigned short new_pos=0;

//////////////В ОСНОВНОМ НОВЫЕ ФУНКЦИИ//////////////////////////

void keyb_handler();
void keyb_process_keys();
char scan_code_table[] = {0,0,'1','2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',0,'\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',' ',0,'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '<','>','+',0,'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',0,'*',0,' ',0, 0,0,};
char letters[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z','A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
void screen_clear();
void on_key(unsigned char scan_code);
void cursor_moveto(unsigned int strnum, unsigned int pos);
void comand_func(unsigned char scan_code);
bool strcmp(unsigned char *str1, const char *str2);
int strlen(unsigned char *str, bool flag);
char c[42]={0};
short str_cnt = 0, symbol_cnt=0;
bool flag=true, shift_flag=false;
int skip_table[255]={0};
void bm(unsigned char* str);
///////////////////////////////////////
void default_intr_handler();
typedef void (*intr_handler)();

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short
flags, intr_handler hndlr);
void intr_init();
void intr_start();
void intr_enable();
void intr_disable();
void out_str(int color, const char* ptr, unsigned int strnum);
static inline unsigned char inb (unsigned short port);
static inline void outb (unsigned short port, unsigned char data);
static inline void outw (unsigned int port, unsigned int data);
void keyb_handler();
void keyb_init();
void keyb_process_keys();
//////////////////////////////////////

// Структура описывает данные об обработчике прерывания
struct idt_entry
{
	 unsigned short base_lo; // Младшие биты адреса обработчика
	 unsigned short segm_sel; // Селектор сегмента кода
	 unsigned char always0; // Этот байт всегда 0
	 unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
	 unsigned short base_hi; // Старшие биты адреса обработчика
} __attribute__((packed)); // Выравнивание запрещено
// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr
{
	 unsigned short limit;
	 unsigned int base;
} __attribute__((packed)); // Выравнивание запрещено

struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt


void outb_symbol(int color, unsigned char symbol){

	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 2*(str_cnt*VIDEO_WIDTH + symbol_cnt);
	video_buf[0] = symbol; // Символ (код)
	video_buf[1] = color; // Цвет символа и фона
	cursor_moveto(str_cnt, symbol_cnt);
	//return;

}
void out_word(int color, const char* ptr)
{
	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 2*(80 * str_cnt + symbol_cnt);
	while (*ptr)
	{
		video_buf[0] = (unsigned char) *ptr; // Символ (код)
		video_buf[1] = color; // Цвет символа и фона
		video_buf += 2;
		symbol_cnt++;
		ptr++;
	}
}

void on_key(unsigned char scan_code){
	if(flag){
		flag=false;
		screen_clear();
		str_cnt=0;
		symbol_cnt=0;
		out_str(0x0A, "# ", 1);
		symbol_cnt=2;
		cursor_moveto(str_cnt,symbol_cnt);
	}
	if (str_cnt>23){
		screen_clear();
		out_str(0x0A, "# ", 1);
		str_cnt=1;
		symbol_cnt=2;
	}
	else if (scan_code == 28){ //enter
		comand_func(scan_code);
		out_str(0x0A, "# ", ++str_cnt);
		symbol_cnt=2;
		str_cnt--;
		cursor_moveto(str_cnt,symbol_cnt);
	}
	else if (scan_code == 14){ //удаление буквы
		if (symbol_cnt >2){
			unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
			video_buf += 2*(str_cnt*VIDEO_WIDTH + (symbol_cnt-1));
			video_buf[0] = '\0';
			cursor_moveto(str_cnt,--symbol_cnt);
		}
	}
	else if (scan_code == 57){ //пробел 
		if (symbol_cnt<42){
			unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
			video_buf += 2*(str_cnt*VIDEO_WIDTH + (symbol_cnt));
			video_buf++;
			cursor_moveto(str_cnt,++symbol_cnt);
		}
	}
	else if (scan_code == 42){ //проверка на шифт для заглавных букв
		shift_flag = true;
	}
	else if (symbol_cnt<42){ //печать символа
		if (shift_flag == false){
			char c = scan_code_table[(unsigned int)scan_code];
			outb_symbol(0x0A, c);
			symbol_cnt++;
		}
		else{
			char c = scan_code_table[(unsigned int)scan_code];
			int i=0;
			for(i;i<26;i++){
				if(c==letters[i]){
					break;
				}
			}
			outb_symbol(0x0A, letters[i + 26]);
			cursor_moveto(str_cnt,++symbol_cnt);
			shift_flag=false;
		}
	}

}
void out_num(int num)
{
  int first;
  if (!num) { outb_symbol(0x0A, '0'); return; }
  while(num)
  {
	if (num > 9){
		 first = num/10;
		 num%=10;
	}
	else { first = num; num/=10;}
	switch(first)
  	{
		case 1: { outb_symbol(0x0A, '1'); break; }
		case 2: { outb_symbol(0x0A, '2'); break; }
		case 3: { outb_symbol(0x0A, '3'); break; }
		case 4: { outb_symbol(0x0A, '4'); break; }
		case 5: { outb_symbol(0x0A, '5'); break; }
		case 6: { outb_symbol(0x0A, '6'); break; }
		case 7: { outb_symbol(0x0A, '7'); break; }
		case 8: { outb_symbol(0x0A, '8'); break; }
		case 9: { outb_symbol(0x0A, '9'); break; }
  	}
   }
}

void comand_func(unsigned char scan_code){

	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 2*(str_cnt*VIDEO_WIDTH + 2);
	
	if (strcmp(video_buf, "shutdown")){
		outw(0x604, 0x2000);
	}
	else if (strcmp(video_buf, "info")){
		if (str_cnt > 18) screen_clear();
		symbol_cnt = 0;
		out_str(0x0A, "        SYSTEM INFO", str_cnt+4);
		out_str(0x0A, "        Variant: 9", ++str_cnt);
		out_str(0x0A, "        OS:  Linux", ++str_cnt);
		out_str(0x0A, "     Translator: FASM", ++str_cnt);
		out_str(0x0A, "       Compiler: gcc", ++str_cnt);
		out_str(0x0A, "  Author: Karina Maronova", ++str_cnt);
		if (*(char*)0x255=='d'){
			out_str(0x0A, "You have chosen STD algorythm", ++str_cnt);
		}
		if (*(char*)0x255=='m'){
			out_str(0x0A, "You have chosen BM algorythm", ++str_cnt);
		}
	}
	else if (strcmp(video_buf, "upcase")){
		unsigned char *str = video_buf + 14;
		int len = strlen(str,true);

		symbol_cnt = 0;
		cursor_moveto(++str_cnt, symbol_cnt);

		while (len > 0) {
		    char c = *str;
		    if (c >= 'a' && c <= 'z') {
		        c -= 'a' - 'A'; // преобразование в верхний регистр
		    }

		    outb_symbol(0x0A, c);
		    cursor_moveto(str_cnt, ++symbol_cnt);
		    str += 2;
		    len--;
		}
	}
	else if (strcmp(video_buf, "downcase")){
		unsigned char *str = video_buf + 18;
		int len = strlen(str,true);

		symbol_cnt = 0;
		cursor_moveto(++str_cnt, 0);

		while (len > 0) {
		    char c = *str;
		    if (c >= 'A' && c <= 'Z') {
		        c += 'a' - 'A'; // преобразование в нижний регистр
		    }

		    outb_symbol(0x0A, c);
		    cursor_moveto(str_cnt, ++symbol_cnt);
		    str += 2;
		    len--;
		}
	}
	else if (strcmp(video_buf, "titlize")){
		unsigned char *str = video_buf + 16;
		int len = strlen(str,true);

		symbol_cnt = 0;
		cursor_moveto(++str_cnt, 0);

		while (len > 0)
		{
			char c = *str;
			if (*(str - 2) == '\0') {//если предыдущий символ был "пробел"
            			if (c >= 'a' && c <= 'z') {
                		    c -= 'a' - 'A'; // преобразование в верхний регистр
            			}
				
			}
			outb_symbol(0x0A, c);
			cursor_moveto(str_cnt, ++symbol_cnt);
			str += 2;
			len--;
		}
	}
	else if (strcmp(video_buf, "template")){
		unsigned char *str = video_buf + 18;
		
		
		int len = strlen(str,true);
		for(int i = 0; i < len; i++){
			c[i] = str[2*i];
		}
		symbol_cnt = 0;
		out_word(0x0A, "Template '");
		out_word(0x0A, c);
		out_word(0x0A, "' loaded. ");
		for (int i = 0; i < 255; i++){
			skip_table[i] = len;
			
		}
		for (int i = 0; i < len - 1; i++) {
			skip_table[(unsigned char)c[i]] = len - 1 - i;
		}
        	//ВЫВОД СМЕЩЕНИЙ 
        	if(*(char*)0x255=='m'){
			out_word(0x0A, "BM info:");
			symbol_cnt = 0;
			cursor_moveto(++str_cnt, symbol_cnt);
		
	
			for (int i = 0; i < len; i++)
			{
				
				outb_symbol(0x0A, c[i]);
				cursor_moveto(str_cnt, ++symbol_cnt);
				outb_symbol(0x0A, ':');
				cursor_moveto(str_cnt, ++symbol_cnt);
				out_num(skip_table[c[i]]);
				cursor_moveto(str_cnt, ++symbol_cnt);
				outb_symbol(0x0A, ' ');
				cursor_moveto(str_cnt, ++symbol_cnt);
				
			}
		}
	}
	else if (strcmp(video_buf, "search")){
		
		bm((video_buf + 14));
	}
	else{
		out_str(0x0C, "WRONG COMMAND!                  ", str_cnt);
		str_cnt--;
	}
	

}

void bm(unsigned char* str){
	char tmp_str[126]={0};
	int text_len = strlen(str,true);
	for(int i = 0; i < text_len; i++){
		tmp_str[i] = str[2*i];
	}
	int pattern_len = strlen((unsigned char*)c,false);


	int i = pattern_len - 1; // Текущая позиция в тексте
	int j = pattern_len - 1; // Текущая позиция в образце

    	while (i < text_len) {
        	j = pattern_len - 1;
        	while (j >= 0 && tmp_str[i] == c[j]) {
           		i--;
            		j--;
        	}	

        	if (j < 0) {
		    	symbol_cnt = 0;
			cursor_moveto(++str_cnt, symbol_cnt);
			out_word(0x0A, "Found '");
			out_word(0x0A, c);
			out_word(0x0A, "' at pos:");
			out_num(i+1);
		    	return;
        	}

        	i += skip_table[(unsigned char)tmp_str[i]];
    	}

   	
	symbol_cnt = 0;
	cursor_moveto(++str_cnt, symbol_cnt);
	out_word(0x0A, "Not found");
	
}

// Функция переводит курсор на строку strnum (0 –самая верхняя) в позицию pos на этой строке (0 –самое левое положение).
void cursor_moveto(unsigned int strnum, unsigned int pos){

	new_pos = (strnum * VIDEO_WIDTH) + pos;
	outb(CURSOR_PORT, 0x0F);
	outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
	outb(CURSOR_PORT, 0x0E);
	outb(CURSOR_PORT + 1, (unsigned char)( (new_pos >> 8) & 0xFF));
}

//////////////////////////////////// MAIN //////////////////////////////////////////////////////////
extern "C" int kmain()
{
	const char* hello = "                    Welcome to StringdOS (gcc edition)!";
	const char* g_test = "                         Created by Karina Maronova";
	// Вывод строки
	unsigned char *video_buf = (unsigned char*) VIDEO_BUF_PTR;
	screen_clear();
	out_str(0x0D, hello, 9);
	out_str(0x0D, g_test, 10);
	cursor_moveto(80,25);

	intr_disable();
	intr_init();
	keyb_init();
	intr_start();
	intr_enable();

// Бесконечный цикл
while(1)
{
	asm("hlt");
}
return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

void screen_clear(){
	
	unsigned char *video_buf = (unsigned char*) VIDEO_BUF_PTR;
	for (int i = 0; i < 80*25; i++) //ширина и высота
	{
	    *(video_buf + i*2) = '\0';   //Каждый элемент занимает 2 байта: аски код и атрибут - цвет
	}
    	cursor_moveto(1, 3);
    	intr_disable();
	intr_init();
	keyb_init();
	intr_start();
	intr_enable();

}

// Пустой обработчик прерываний. Другие обработчики могут быть реализованы по этому шаблону
void default_intr_handler()
{
asm("pusha");
// ... (реализация обработки)
asm("popa; leave; iret");
}

typedef void (*intr_handler)();

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short
flags, intr_handler hndlr)
{
	unsigned int hndlr_addr = (unsigned int) hndlr;
	g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF);
	g_idt[num].segm_sel = segm_sel;
	g_idt[num].always0 = 0;
	g_idt[num].flags = flags;
	g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16);
}
// Функция инициализации системы прерываний: заполнение массива с адресами обработчиков
void intr_init()
{
	 int i;
	 int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	 for(i = 0; i < idt_count; i++)
	 intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
	 default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}

void intr_start()
{
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	g_idtp.base = (unsigned int) (&g_idt[0]);
	g_idtp.limit = (sizeof (struct idt_entry) * idt_count) - 1;
	asm("lidt %0" : : "m" (g_idtp) );
}

void intr_enable()
{
	asm("sti");
}
void intr_disable()
{
	asm("cli");
}

// функция для вывода строки
void out_str(int color, const char* ptr, unsigned int strnum)
{
	unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
	video_buf += 80*2 * strnum;
	while (*ptr)
	{
		video_buf[0] = (unsigned char) *ptr; // Символ (код)

		video_buf[1] = color; // Цвет символа и фона
		video_buf += 2;
		ptr++;
		
	}
	str_cnt++;
}

static inline unsigned char inb (unsigned short port) // Чтение из порта
{
	unsigned char data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
	return data;
}

static inline void outb (unsigned short port, unsigned char data) // Запись
{
	asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline void outw (unsigned int port, unsigned int data)
{
	asm volatile ("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

void keyb_init(){
	intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
 	outb(PIC1_PORT+ 1, 0xFF^ 0x02); 
}

void keyb_handler(){
	asm("pusha");// Обработка поступивших данных
	keyb_process_keys();
	outb(PIC1_PORT, 0x20); 
	asm("popa; leave; iret");
}

void keyb_process_keys(){
// Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
	if (inb(0x64) & 0x01) {
		unsigned char scan_code;
		unsigned char state;
		scan_code= inb(0x60);
		if(scan_code< 128) on_key(scan_code);
	}
}

//////////////////////// РАБОТА СО СТРОКАМИ ///////////////////////////////////////////
bool strcmp( unsigned char *str1, const char *str2){
    while (*str1 != '\0' && *str1 != ' ' && *str2 != '\0' && *str1 == *str2) 
    {
      str1+=2;
      str2++;
    }

   if (*str1 == *str2) return true; 
   return false;
}
int strlen(unsigned char *str, bool flag){
	int i = 0;
	if(flag){
		
		while(*str != '\0'||*(str+2)!='\0') { str+=2; i++;}
	
	}
	else{

		while(*str != '\0') { str++; i++;}
	}
	return i;
}
////////////////////////////////////////////////////////////
