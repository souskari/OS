use16
org 0x7C00
start:
;  Инициализация  адресов  сегментов.  Эти  операции  требуется  не  для любого BIOS, но их рекомендуется проводить.
mov ax, cs ; Сохранение адреса сегмента кода в ax 
mov ds, ax; Сохранение этого адреса как начало сегмента данных
mov ss, ax ; И сегмента стека
mov sp, start ; Сохранение адреса стека как адрес первой инструкции этого кода. Стек будет расти вверх и не перекроет код.


mov ax, cs ; Сохранение адреса сегмента кода в ax 
mov ds, ax; Сохранение этого адреса как начало сегмента данных
mov ss, ax ; И сегмента стека
mov sp, start ; Сохранение адреса стека как адрес первой инструкции этого кода. Стек будет расти вверх и не перекроет код.
mov ah,  0x0e

    mov al, ' '
    int 0x10
    int 0x10
    mov al, 's'
    int 0x10
    mov al, 't'
    int 0x10
    mov al, 'd'
    int 0x10
    mov al, '/'
    int 0x10
    mov al, 'b'
    int 0x10
    mov al, 'm'
    int 0x10
    mov al, '?'
    int 0x10
    mov al, ' '
    int 0x10

wait_for_letter_s_cycle:

    mov ah, 0x00 ; регистровая пара ah и al. Команда мув записывает значения в al (поэтому ниже проверяем именно его)
    int 0x16
    cmp al, 's'
    je wait_for_letter_t_cycle
    cmp al, 'b'
    je wait_for_letter_m_cycle
    jmp wait_for_letter_s_cycle
    
wait_for_letter_t_cycle:

    mov ah, 0x00
    int 0x16
    cmp al, 't'
    je wait_for_letter_d_cycle
    jne wait_for_letter_s_cycle
    
wait_for_letter_d_cycle:

    mov ah, 0x00
    mov cx, 'd'
    mov [0x255], cx
    int 0x16
    cmp al, 'd'
    je load_drive
    jne wait_for_letter_s_cycle
    
wait_for_letter_m_cycle:

    mov ah, 0x00
    mov cx, 'm'
    mov [0x255], cx
    int 0x16
    cmp al, 'm'
    je load_drive
    jmp wait_for_letter_s_cycle

;окончание проверки на std\bm

load_drive:

    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    mov al, 0x11 ; sum of sectors to read
    mov dl, 0x01 ; number of drive
    mov dh, 0x00 ; number of head
    mov ch, 0x00 ; number of track(cylinder)
    mov cl, 0x01 ; number of sector
    mov ah, 0x02 ; function for int 0x13
    int 0x13 ; read data from 0x1000


; Отключение прерываний
	cli
; Загрузка размера и адреса таблицы дескрипторов
lgdt[gdt_info]

; Включение адресной линии А20
in al, 0x92
or al, 2
out 0x92, al
; Установка бита PE регистра CR0 -процессор перейдет в защищенный режим
mov eax, cr0
or al, 1
mov cr0, eax
jmp 0x8:protected_mode; "Дальний" переход для загрузки корректной информации в cs(архитектурные особенности не позволяют этого сделать напрямую).




gdt:
	; Нулевой дескриптор
	db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	
	; Сегмент кода: base=0, size=4Gb, P=1, DPL=0, S=1(user), 
	; Type=1(code), Access=00A, G=1, B=32bit
	db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00 

	; Сегмент данных: base=0, size=4Gb, P=1, DPL=0, S=1(user), 
	; Type=0(data), Access=0W0, G=1, B=32bit
	db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

gdt_info: ; Данные о таблице GDT(размер, положение в памяти)
	dw gdt_info - gdt   ; Размер таблицы (2 байта)
	dw gdt, 0           ; 32-битный физический адрес таблиц



use32
protected_mode:
	; Загрузка селекторов сегментов для стека и данных в регистры
	mov ax, 0x10 ; Используется дескриптор с номером 2 в GDT
	mov es, ax
	mov ds, ax
	mov ss, ax
	;Передача управления загруженному ядру
	call 0x10000 ; Адрес равен адресу загрузки в случае если ядро скомпилировано в "плоский" код
inf_loop:jmp inf_loop ; Бесконечный цикл

times (512 -($ -start) -2) db 0
db 0x55, 0xAA 
