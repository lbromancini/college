#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

// Modo manual e automático

#define MODO_AUTO ((PINC&(1<<PC0))) // Sinal lógico 0
#define MODO_MANUAL (!(PINC&(1<<PC0))) // Sinal lógico 1

uint8_t estado = 1; // 1 = tanque com tampão | 2 = tanque com efluente | 3 = tanque vazio
uint8_t tarefa = 1; // 1 = aferir ph | 2 = controlar ph
uint8_t ph = 7;
uint8_t cont_afericao = 0; // Contagem de tempo para a aferição de pH

ISR (TIMER1_COMPA_vect) { // Contador para o início da aferição de pH

  cont_afericao = cont_afericao + 1;

  TIFR1 = 0;
  TCNT1 = 0; // Zera o timer

}

ISR(INT0_vect) { // Interrupção para aferição de pH manual

  if (MODO_MANUAL) {

    afericao_ph(ph);
    
    } 
  
}


int main (void) {

Serial.begin(9600); // Inicializa a porta serial

Serial.print("Modo de funcionamento: ");
    
    if (MODO_AUTO) {

      Serial.println("Automático"); 
      
    } else if (MODO_MANUAL) {

      Serial.println("Manual"); 

    }

TCCR1A = 0;
TCCR1B = 0;

uint8_t nivel = 0;

// Configuração pinos

DDRA |= (1<<PA1)|(1<<PA3)|(1<<PA5)|(1<<PA7); // PA1 (23), PA3 (25), PA5 (IN1 - 27) e PA7 (IN2 - 29) como saídas
DDRC |= (1<<PC4)|(1<<PC6); // PC4 (IN4 - 33), PC6 (IN3 - 31), 
PORTC |= (1<<PC2)||(1<<PC0); // Pull up em PC2 (Sensor de boia)(35) e PC0 (Botão manual/auto)(37)

// Timer para a aferição de pH

TCCR1B |= (1<<WGM12); // Timer em modo CTC
TCCR1B |= (1<<CS10)|(1<<CS12); // Prescaler 128
OCR1A = 15624; // OCR1A para o tempo de 1s
TIMSK1 |= (1<<OCIE1A); // Habilita a interrupção por timer COMPA

// Interrupção para realizar a aferição manual

PORTD |= (1<<PD0); // Pull up em PD0
EICRA |= (1<<ISC01); // Configura a interrupção INT0 pra borda de descida
EIMSK |= (1<<INT0); // Habilita a interrupção INT0
sei(); // Habilita as interrupções

  while(1) {
      
    if (MODO_MANUAL) {

      TIMSK1 &=~ (1<<OCIE1A); // Desabilita a interrupção por timer COMPA no modo manual
      
    }

    // Controle de nível e vazão

    // Tarefas do sistema

    if (tarefa == 1) {

      if (estado == 1) { // Tanque de aferição com tampão

      // Realiza a aferição de pH se estiver em modo automático e se o contador atingir o tempo definido

        if (cont_afericao >= 10 && MODO_AUTO) { 

           afericao_ph(ph);
           cont_afericao = 0;

        }

      }
      
        else if (estado == 2) { // Tanque de aferição com efluente

        // Esvazia o tanque e enche com solução tampão, e aguarda a próxima aferição

        esvazia_efluente();
        enche_tampao();
        estado = 1;

        }
      
        else if (estado == 3) { // Tanque de aferição vazio

        // Enche o tanque com solução tampão, e aguarda a próxima aferição

        enche_tampao();
        estado = 1;

        }

    }

    else if (tarefa == 2) {

      if (ph < 4) {

        controle_ph_acido(); // Se o pH estiver básico, realiza o controle com reagente ácido

      } else if (ph > 9) {

        controle_ph_basico(); // Se o pH estiver ácido, realiza o controle com reagente básico

      } else if (ph >= 4 || ph <= 9) {

        controle_ph_feito();

      }

    }


  } // while (1)

} // int main (void)

// Funções

// Funções de acionamento dos motores

void liga_p102(void) {

  PORTA |= (1<<PA1);

}
void desliga_p102(void) {

  PORTA &=~ (1<<PA1);

}

void liga_p103(void) {

  PORTA |= (1<<PA3);

}
void desliga_p103(void) {

  PORTA &=~ (1<<PA3);

}

void liga_p104_in1(void) {

  PORTA |= (1<<PA7);

}
void desliga_p104_in1(void) {

  PORTA &=~ (1<<PA7);

}

void liga_p104_in2(void) {

  PORTA |= (1<<PA5);

}
void desliga_p104_in2(void) {

  PORTA &=~ (1<<PA5);

}

void liga_p105_in3(void) {

  PORTC |= (1<<PC6);

}
void desliga_p105_in3(void) {

  PORTC &=~ (1<<PC6);

}

void liga_p105_in4(void) {

  PORTC |= (1<<PC4);

}
void desliga_p105_in4(void) {

  PORTC &=~ (1<<PC4);

}

// Checagem de estado do sensor de nível por boia

#define BOIA_ATIVADA ((PINC&(1<<PC2))) // Sinal lógico 1
#define BOIA_DESATIVADA (!(PINC&(1<<PC2))) // SInal´lógico 0

// Tarefas do sistema

void esvazia_tampao(void) {

  Serial.println("Tanque esvaziando de tampão"); 
  liga_p105_in4();
  _delay_ms(75000); // espera o tanque esvaziar
  desliga_p105_in4();
  _delay_ms(250); // delay pra ponte H não dar curto

}

void esvazia_efluente(void) {

  Serial.println("Tanque esvaziando de efluente");
  liga_p104_in2();
  _delay_ms(75000); // espera o tanque esvaziar
  desliga_p104_in2();
  Serial.println("Tanque esvaziado");
  _delay_ms(250); // delay pra ponte H não dar curto

}

void enche_tampao(void) {

  Serial.println("Tanque enchendo de tampão");
  liga_p105_in3();
  while BOIA_DESATIVADA;
  desliga_p105_in3();
  Serial.println("Tanque cheio de tampão");

}

void enche_efluente(void) {

  Serial.println("Tanque enchendo de efluente");
  liga_p104_in1();
  while BOIA_DESATIVADA;
  desliga_p104_in1();
  Serial.println("Tanque cheio de efluente");

}

void afericao_ph(int p) {

  Serial.println("Realizando aferição de pH");
  esvazia_tampao();
  enche_efluente();
  // AnalogRead do pH
  Serial.println("Aferindo pH...");
  _delay_ms(2000); // Tempo para realizar as aferições
  esvazia_efluente();
  enche_tampao();
  Serial.print("pH = ");
  Serial.println(p);
  
  if (p <= 4) {
    
    Serial.println("pH muito ácido, controle será necessário");
    tarefa = 2;
    
  } else if (p >= 9) {
    
    Serial.println("pH muito básico, controle será necessário");
    tarefa = 2;
    
  } else {
    
    Serial.println("pH ideal, o controle não será necessário");
    
  }

}

void controle_ph_feito(void) {

  Serial.println("Controle de pH finalizado!");
  desliga_p102();
  desliga_p103();
  tarefa = 1;

}

void controle_ph_basico(void) {

  Serial.println("Realizando controle de pH com reagente básico");
  liga_p102();
  _delay_ms(5000);
  // delay com o tempo calculado de acordo com o ph
  // Após terminar o controle...
  controle_ph_feito();

}

void controle_ph_acido(void) {

  Serial.println("Realizando controle de pH com reagente ácido");
  liga_p103();
  _delay_ms(5000);
  // delay com o tempo calculado de acordo com o ph
  // Após terminar o controle...
  controle_ph_feito();

}
