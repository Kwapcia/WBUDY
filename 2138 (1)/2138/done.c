/*************************************************************************************
 *
 * @Description:
 * Program przykładowy - odpowiednik "Hello World" dla systemów wbudowanych
 * Rekomendujemy wkopiowywanie do niniejszego projektu nowych funkcjonalności
 *
 *
 * UWAGA! Po zmianie rozszerzenia na cpp program automatycznie będzie używać
 * kompilatora g++. Oczywiście konieczne jest wprowadzenie odpowiednich zmian w
 * pliku "makefile"
 *
 *
 * Program przykładowy wykorzystuje Timer #0 i Timer #1 do "mrugania" diodami
 * Dioda P1.16 jest zapalona i gaszona, a czas pomiędzy tymi zdarzeniami
 * odmierzany jest przez Timer #0.
 * Program aktywnie oczekuje na upłynięcie odmierzanego czasu (1s)
 *
 * Druga z diod P1.17 jest gaszona i zapalana w takt przerwań generowanych
 * przez timer #1, z okresem 500 ms i wypełnieniem 20%.
 * Procedura obsługi przerwań zdefiniowana jest w innym pliku (irq/irq_handler.c)
 * Sama procedura MUSI być oznaczona dla kompilatora jako procedura obsługi 
 * przerwania odpowiedniego typu. W przykładzie jest to przerwanie wektoryzowane.
 * Odpowiednia deklaracja znajduje się w pliku (irq/irq_handler.h)
 * 
 * Prócz "mrugania" diodami program wypisuje na konsoli powitalny tekst.
 * 
 * @Authors: Julia Pietrzykowska
 * 			Karolina Soltysiak
 * 			Aleksandra Kwapinska
 *
 * @Change log:
 *           2016.12.01: Wersja oryginalna.
 *
 ******************************************************************************/

#include "general.h"
#include <lpc2xxx.h>
#include <printf_P.h>
#include <printf_init.h>
#include <consol.h>
#include <config.h>
#include "irq/irq_handler.h"
#include "timer.h"
#include "VIC.h"
#include "lcd.h"
#include "Common_Def.h"
#include <stdio.h>


/************************************************************************
 * @Description: opóźnienie wyrażone w liczbie sekund
 * @Parameter:
 *    [in] seconds: liczba sekund opĂłĹşnienia
 * @Returns: Nic
 * @Side effects:
 *    przeprogramowany Timer #0
 *************************************************************************/
 void sdelay (tU32 seconds)
{
	T0TCR = TIMER_RESET;                    //Zatrzymaj i zresetuj
    T0PR  = PERIPHERAL_CLOCK-1;             //jednostka w preskalerze
    T0MR0 = seconds;
    T0IR  = TIMER_ALL_INT;                  //Resetowanie flag przerwaĹ„
    T0MCR = MR0_S;                          //Licz do wartości w MR0 i zatrzymaj się
    T0TCR = TIMER_RUN;                      //Uruchom timer

    // sprawdź czy timer działa
    // nie ma wpisanego ogranicznika liczby pętli, ze względu na charakter procedury
    while (T0TCR & TIMER_RUN)
    {
    }
}

 void delayMs(tU16 delayInMs) {
     T1TCR = 0x02;
     T1PR = 0x00;
     T1MR0 = delayInMs * (CORE_FREQ / PBSD / 1000);
     T1IR = 0xff;
     T1MCR = 0x04;
     T1TCR = 0x01;

     while ((T1TCR & 0x01) == 1) {
     }
 }
/************************************************************************
 * @Description: uruchomienie obsługi przerwań
 * @Parameter:
 *    [in] period    : okres generatora przerwań
 *    [in] duty_cycle: wypełnienie w %
 * @Returns: Nic
 * @Side effects:
 *    przeprogramowany timer #1
 *************************************************************************/
static void init_irq (tU32 period, tU8 duty_cycle)
{
	//Zainicjuj VIC dla przerwań od Timera #1
    VICIntSelect &= ~TIMER_1_IRQ;           //Przerwanie od Timera #1 przypisane do IRQ (nie do FIQ)
    VICVectAddr5  = (tU32)IRQ_Test;         //adres procedury przerwania
    VICVectCntl5  = VIC_ENABLE_SLOT | TIMER_1_IRQ_NO;
    VICIntEnable  = TIMER_1_IRQ;            // Przypisanie i odblokowanie slotu w VIC od Timera #1
  
    T1TCR = TIMER_RESET;                    //Zatrzymaj i zresetuj
    T1PR  = 0;                              //Preskaler nieużywany
    T1MR0 = ((tU64)period)*((tU64)PERIPHERAL_CLOCK)/1000;
    T1MR1 = (tU64)T1MR0 * duty_cycle / 100; //Wypełnienie
    T1IR  = TIMER_ALL_INT;                  //Resetowanie flag przerwań
    T1MCR = MR0_I | MR1_I | MR0_R;          //Generuj okresowe przerwania dla MR0 i dodatkowo dla MR1
    T1TCR = TIMER_RUN;                      //Uruchom timer
}

//void mruganie(int freq, int time){
//}

int rand_X = 23;
int rand_m = 150;
int rand_c = 43;
int rand_a = 21;
int rand(){
	rand_X = (rand_a * rand_X + rand_c) % rand_m;
	return rand_X;
}

int main(void)
{
	//uruchomienie 'simple printf'
    printf_init();
    lcdInit();
       lcdColor(0xff,0x00);
    //powitanie
    simplePrintf("\n\n\n\n");
    simplePrintf("\n*********************************************************");
    simplePrintf("\n*");
    simplePrintf("\n* Systemy Wbudowane");
    simplePrintf("\n* Wydzial FTIMS");
    simplePrintf("\n* Moj pierwszy program");
    simplePrintf("\n*");
    simplePrintf("\n*********************************************************");

    /*********************************************************************
     * Ta część inicjuje działanie programy
     *********************************************************************/
    // uruchomienie GPIO na nodze P1.16 i P1.17.
    PINSEL2 &= ~(1 << 3);
    // Kierunek out na nodze P1.16
    IODIR1 |= DIODA_LEWA; //(1 << 16);
    // Kierunek out na nodze P1.17
    IODIR1 |= DIODA_PRAWA; //(1 << 17);
    // Uruchomienie przerwań co 1/2 s.
    init_irq(500, 20);

    /**********************************************************************
     * Ta część jest nieskończoną pętlą realizującą działania programu
     * Należy jednak pamiętać, że część jego funkcjonalności realizowana
     * jest za pomocą przerwań
     **********************************************************************/
    // Aktywne "mruganie" diodą
    //tBool dioda_swieci = FALSE;
    int wybor = 0;
    while (TRUE)
    {
    	lcdClrscr();
    	while((IOPIN & 0x00000100)!=0)
        {
        lcdGotoxy(0,0);
        lcdPuts("UwUOwOUwUOwOUwU\n");
        lcdPuts("SUPER COOL GAME\n");
        lcdPuts("***************\n");
        lcdPuts("<-PLAY   NAME->\n");
        if((IOPIN & 0x00000200)==0)
        {
            lcdPuts("\n      PLAY\n");
            lcdPuts(" press joystick");
            wybor = 2;
        }


        if((IOPIN & 0x00000800)==0)
        {
          lcdPuts("\n      NAME\n");
          lcdPuts(" press joystick");
          wybor = 1;
        }

        }

    	if(wybor==2){
    		 lcdClrscr();
    		 lcdGotoxy(10, 0);
    		 lcdPuts("game here");
    		 delayMs(1000);
    		 wybor = 0;

    		 IODIR1 |= 1 << 16;
    		 IODIR1 |= 1 << 17;
    		 IODIR1 |= 1 << 18;
    		 IODIR1 |= 1 << 19;

    		 bool isGame = true;
    		 bool isStupid = false;
    		 int round_number = 0;
    		 long int time = 0;
    		 while(isGame){
    			 int random_number = rand() % 4;
    			 cos |= 1 << random_number + jakisoffset;
    			 bool roundInProgress = true;
    			 while(roundInProgress){
    				 int choice = (IOPIN & 0x00001F00) >> 8;
    				 if(choice){
    					 if(choice == 1<<random_number+1){
    						 // User selected correct input
    						 round_number += 1;
    						 roundInProgress = false;
    					 }else{
    						 isStupid = true;
    						 isGame = false;
    						 roundInProgres = false;
    					 }
    				 }else{
    					 delayMs(10);
    					 time += 10;
    				 }

    			 }
    			 cos &=(0 << random_number + jakisoffset);

    		 }
    	}

        if(wybor == 1)
        {
        lcdClrscr();
        while((IOPIN & 0x00000100)!=0){
        }
        lcdClrscr();
         lcdGotoxy(10, 0);
         lcdPuts("UP/DOWN-choice\n");
         lcdPuts(" RIGHT-next\n");
         lcdPuts(" PRESS=OK\n");
         lcdPuts(" NAME:\n");

         char name[12];  // rozmiar = rozmiar nazwy + suma kontrolna na 11 i 12
         // bajcie

         int index = 0;
         char sign = 'A';
         int position = 10;

         lcdGotoxy(position, 60);
         lcdPutchar(sign);
         delayMs(500);

         while ((IOPIN & 0x00000100) != 0) {  // zatwierdzenie srodkowy
        	 delayMs(200);
             // check if P0.12 down-key is pressed
             if ((IOPIN & 0x00001000) == 0) {
                 if (sign == 'A') {
                     sign = 'Z';
                 } else {
                     sign--;
                 }
                 lcdGotoxy(position, 60);
                 lcdPutchar(sign);
             }
                 // check if P0.10 up-key is pressed
             else if ((IOPIN & 0x00000400) == 0) {
                 if (sign == 'Z') {
                     sign = 'A';
                 } else {
                     sign++;
                 }
                 lcdGotoxy(position, 60);
                 lcdPutchar(sign);
             }
                 // check if P0.9 left-key is pressed
             else if ((IOPIN & 0x00000200) == 0) {
                 name[index] = '\0';
                 if (index != 0) {
                     index--;
                 }
                 if (position != 10) {
                     lcdGotoxy(position, 60);
                     lcdPutchar(' ');
                     position -= 10;
                     lcdGotoxy(position, 60);
                     lcdPutchar('A');
                     sign = 'A';
                 }
             }
                 // check if P0.11 right-key is pressed
             else if ((IOPIN & 0x00000800) == 0) {
                 if (position != 90) {  // jesli to nie jest ostatni znak
                     name[index] = sign;
                     index++;
                     position += 10;
                     lcdGotoxy(position, 60);
                     lcdPutchar(sign);
                 }
             } else {
             }
         }
         name[index] = sign;
         while (index < 12) {
             index++;
             name[index] = '\0';
         }
         lcdGotoxy(10, 80);
         lcdPuts("YOUR NAME:\n");
         lcdGotoxy(10, 95);
         lcdPuts(name);
         delayMs(1000);
         wybor = 0;
        }


    }

} 


