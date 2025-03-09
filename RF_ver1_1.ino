#include <Arduino.h>
#include "adf4351.h"

unsigned int akk[10];
byte print_counter=0;
float corr=0;
unsigned int corr_count=0;
byte corr_buf[500];
bool calibrated=0;
bool cold_start=1;
int normalisation_calibr = 0;
float ema,smoothedValue,smoothedValue2;

unsigned long frq_start=428000000;        // стартовая частота
unsigned long frq_stop =442000000;        // конечная частота
unsigned long frq_step= (((frq_stop-frq_start)/500 + 250) / 500) * 500;      // автоподбор шага

float EMA_previous = 200; 
float alpha = 0.09;      // Коэффициент сглаживания (0 < alpha <= 1)

int buffer[15]={200,200,200,200,200,200,200,200,200,200,200,200,200,200,200};
int buffer2[20]={200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,};
int index = 0;
int index2 = 0;
long sum = 3000;
long sum2 = 4000;
#define BUFFER_SIZE 15
#define BUFFER_SIZE2 20

ADF4351  vfo(PIN_SS, SPI_MODE0, 1000000UL , MSBFIRST) ;

void setup()
{
  analogReference(INTERNAL2V56);
  Serial.begin(9600);
  Wire.begin() ;
  vfo.pwrlevel = 3 ; 
  vfo.RD2refdouble = 0 ; ///< ref doubler off
  vfo.RD1Rdiv2 = 0 ;   ///< ref divider off
  vfo.ClkDiv = 150 ;
  vfo.BandSelClock = 80 ;
  vfo.RCounter = 25 ;  ///< R counter to 1 (no division)
  vfo.ChanStep = steps[0] ;  /// set to 1 kHz steps

  // частота опорного генератора
  if ( vfo.setrf(25000000UL) ==  0 ){}
  else Serial.println("ref freq set error") ;
  vfo.init() ;
  vfo.enable() ;
  zero_corr_buf();
  }

void loop()
{ vfo.setf(frq_start);
  delay(1000);
  
while(vfo.cfreq<frq_stop) 
{  
         vfo.setf(vfo.cfreq+frq_step); 
         for(int i=0;i<10;i++) akk[i] = analogRead(A2);
              //Serial.print("Исходный массив:"); for (int i = 0; i < 10; i++) {Serial.print(akk[i]);Serial.print(" ");}
         quickSort(akk, 0, 9);
              //Serial.println(' ');Serial.print("Сорт. массив:");for (int i = 0; i < 10; i++) {Serial.print(akk[i]);Serial.print(" ");}   Serial.println(' ');delay(1000);
         unsigned int akk_avr=0;
         for(int i=2;i<8;i++)akk_avr=akk_avr+akk[i];
         float sensorValue=akk_avr/6;
       
     //--------cold_start----------
      if(cold_start || !calibrated ){smoothedValue = movingAverage(sensorValue);smoothedValue2 = movingAverage2(smoothedValue);red();}
      
     //---------normalisation massive forming---------------------- 
       if(!calibrated && !cold_start){
       gr();
       unsigned int norm_value = smoothedValue-normalisation_calibr;
       if(norm_value < 256 && norm_value >=0) corr_buf[corr_count] = norm_value;
       if (norm_value > 255) corr_buf[corr_count]=255;
       if (norm_value < 0) corr_buf[corr_count]=0;       }
             
      //--------------normal------------------- 
       if(calibrated) {smoothedValue = movingAverage(sensorValue) - corr_buf[corr_count]; smoothedValue2 = movingAverage2(smoothedValue);}
       if(corr_count>0)  {Serial.print(smoothedValue);Serial.print(',');Serial.println(smoothedValue2);}
       if(cold_start && corr_count == 40) {
                while((smoothedValue-normalisation_calibr) >100)normalisation_calibr++;
                break;}   // остановка прогона cold_start
        corr_count++;        
}
     if(!cold_start)calibrated=1;
     cold_start=0; 
     corr_count=0;
     blue();  
}

float calculateEMA(float currentValue) {
    float EMA_current = alpha * currentValue + (1 - alpha) * EMA_previous;
    EMA_previous = EMA_current;
    return EMA_current;
}

float movingAverage(int newValue) {
  sum -= buffer[index];
  buffer[index] = newValue;
  sum += newValue;
  index = (index + 1) % BUFFER_SIZE;
  return round((float)sum / BUFFER_SIZE);
}

float movingAverage2(int newValue2) {
  sum2 -= buffer2[index2];
  buffer2[index2] = newValue2;
  sum2 += newValue2;
  index2 = (index2 + 1) % BUFFER_SIZE2;
  return round((float)sum2 / BUFFER_SIZE2);
}

//функция сброса коррекционных коэффициентов 
void zero_corr_buf(){for(int i=0;i<=500;i++){corr_buf[i]=0;}} 

//функция сортировки массива
void quickSort(int arr[], int left, int right) {
  int i = left, j = right; int tmp;
  int pivot = arr[(left + right) / 2];

  // Разделение массива
  while (i <= j) {
    while (arr[i] < pivot)
      i++;
    while (arr[j] > pivot)
      j--;
    if (i <= j) {
      tmp = arr[i];
      arr[i] = arr[j];
      arr[j] = tmp;
      i++;
      j--;
    }
  }

  // Рекурсивный вызов для сортировки подмассивов
  if (left < j)
    quickSort(arr, left, j);
  if (i < right)
    quickSort(arr, i, right);
}

void red(){pinMode(3,OUTPUT);digitalWrite(3,1);delay(2);digitalWrite(3,0);}
void gr(){pinMode(4,OUTPUT);digitalWrite(4,1);delay(2);digitalWrite(4,0);}
void blue(){pinMode(5,OUTPUT);digitalWrite(5,1);delay(2);digitalWrite(5,0);}
