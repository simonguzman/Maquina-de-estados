/**
  * @archivo MaquinaEstadosFinitos.ino
  *
  * @sección  Descripción
  * Maquina de estados que valida una contraseña y 
  * te da acceso a las configuraciones
  *
  * @autor 
  * - Presentado por Julian David Camacho, Simon Guzman Anaya y Karen Yulieth Ruales 
  */
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <StateMachineLib.h>
#include "AsyncTaskLib.h"

//------------Maquina de estado-----------------------
/** Tipo de enumeracion para State*/
// Alias
enum State {
  PosicionA = 0,
  PosicionB = 1,
  PosicionC = 2,
  PosicionD = 3
};

/** Tipo de enumeracion para Input*/
enum Input {  
  Reset=0,
  Forward=1,
  Backward=2,
  Unknown=3,
  };
// Stores last user input
Input input;
//Crear una máquina de estado
StateMachine stateMachine(4, 6);

//------------Conexion lcd-----------------------
LiquidCrystal lcd(12,11,5,4,3,2);

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {41,43,45,47}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {40,42,44}; //connect to the column pinouts of the keypad
//----------------------------------------------------------
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

#define beta 4090 //the beta of the thermistor

//LED
#define RED 9
#define GREEN 8
#define BLUE 7

#define tempPin A0
#define fotoPin A1
#define microPin A2
#define bzz 50

#define DIR_TEMP_ALTA 0
#define DIR_TEMP_BAJA 3
#define DIR_TEMP_ALARMA 6
#define DIR_LUZ 9
#define DIR_MICRO 13

#define FRECUENCIA 440
#define TIEMPO_SONIDO 7000

int pos = 0;
int intentos=0;

int tempAlta = 25;
int tempBaja = 15;
int tempAlarma = 15;
int micro = 30;
int intensidad = 850;

boolean enterMenu = false;

boolean down=false;
unsigned long tiempoInicioKey=0;
unsigned long tiempoFinalKey=0;

void leerTemp();
void luz();

//Asynctask es la opcion para una tarea asincrona de la temperatura
AsyncTask asyncTemperatura(2000, true, leerTemp );
AsyncTask asyncLuz(2000, true, luz );
int outputValue = 0;


const char frame[8][16] = {
  {"UtempHigh      "},
  {"UtempLow       "},
  {"UtempAlarma    "},
  {"Uluz           "},
  {"Usonido        "},
  {"Reset          "},
  {"Atras          "},
  {"Siguiente      "}
};
int page = 0; // Pantallas que se presentan, en este caso 5
int selectedIndex = 0; //Indice o pantalla que se seleccion
int lastFrame = 7; //Ultima pantalla del lcd

char contra[4] = {'1','2','3','4'};
char contraleida[4]={'*','*','*','*'};


void setup(){
  Serial.begin(9600);
 
  // set up the LCD’s number of columns and rows:
  lcd.begin(16, 2);
  //Activa los leds y el boton
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  //Activar el buzzer, o alarma
  pinMode(bzz, OUTPUT);
  configurarStateMachine();
}void loop(){
  actualizarStateMachine();
}

//---------------------------------Máquina de estado---------------------------------
/**
 * Procedimiento que muestra la depuración del estado A referente a la seguridad <br>
 * <b>post: </b> procedimiento que se encarga de hacer el debug de estado A
 */
//Auxiliar output functions that show the state debug
void outputA(){//seguridad
  Serial.println("A   B   C   D");
  Serial.println("X            ");
  Serial.println();
  input=Input::Unknown;
  lcd.clear();
}
/**
 * Procedimiento que muestra la depuración del estado B referente a la configuración <br>
 * <b>post: </b> procedimiento que se encarga de hacer el debug de estado B
 */
void outputB(){//configuracion
  Serial.println("A   B   C   D");
  Serial.println("    X        ");
  Serial.println();
 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Configuracion");
  cargarPorDefecto();
  delay(2000);
  lcd.clear();
  input=Input::Unknown;
}
/**
 * Procedimiento que muestra la depuración del estado C referente al administrador de tareas <br>
 * <b>post: </b> procedimiento que se encarga de hacer el debug de estado C
 */
void outputC(){//taskManager
  Serial.println("A   B   C   D");
  Serial.println("        X    ");
  Serial.println();
  input=Input::Unknown;
  //Inicia las tareas para la temperatura y luz
  lcd.clear();
  asyncTemperatura.Start();
  asyncLuz.Start();
}
/**
 * Procedimiento que muestra la depuración del estado D referente a la alarma <br>
 * <b>post: </b> procedimiento que se encarga de hacer el debug de estado D
 */
void outputD(){//alarm
  Serial.println("A   B   C   D");
  Serial.println("            X");
  Serial.println();
  lcd.clear();
  input=Input::Unknown;
}
//Acciones de los estados
/**
 * Procedimiento que hace accion en el estado A <br>
 * <b>post: </b> procedimiento que se encarga de controlar el acceso
 * @see: controlAcceso()
 */
void actionA(){//control de acceso
  controlAcceso();
}
/**
 * Procedimiento que hace accion en el estado B <br>
 * <b>post: </b> procedimiento que se encarga de configurar el menu
 * @see: controlAcceso()
 * @see: controlConfigs()
 */
void actionB(){//menú de configurar
  //Si entra aqui entonces entro al submenu
    if(enterMenu != true){
      while(enterMenu != true){
        if(input!=3){  
          Serial.println("Saliendo menú principal");
          return;
        }
        controlMenu();
      }
    }else{
      while(enterMenu != false){
        if(input!=3){
          Serial.println("Saliendo menú principal");
          return;
        }
        controlConfigs();
      }
    }
}
/**
 * Procedimiento que hace accion en el estado C <br>
 * <b>post: </b> procedimiento que se encarga de las tareas de los sensores
 * @see: actualizarTareas()
 */
void actionC(){//Control de tareas de sensores
  actualizarTareas();
}
/**
 * Procedimiento que hace accion en el estado D <br>
 * <b>post: </b> procedimiento que se encarga de las alarmas
 * @see: alarma()
 */
void actionD(){//Alarma
  alarma();
  delay(4000);
}
//Configurar maquina de estado
void configurarStateMachine(){
  Serial.begin(9600);
  // Add transitions
  stateMachine.AddTransition(PosicionA, PosicionB, []() { return input == Forward; });
  stateMachine.AddTransition(PosicionB, PosicionA, []() { return input == Backward; });
  stateMachine.AddTransition(PosicionB, PosicionC, []() { return input == Forward; });
  stateMachine.AddTransition(PosicionC, PosicionB, []() { return input == Backward; });
  stateMachine.AddTransition(PosicionC, PosicionD, []() { return input == Forward; });
  stateMachine.AddTransition(PosicionD, PosicionB, []() { return input == Forward; });
  // Add actions
  stateMachine.SetOnEntering(PosicionA, outputA);
  stateMachine.SetOnEntering(PosicionB, outputB);
  stateMachine.SetOnEntering(PosicionC, outputC);
  stateMachine.SetOnEntering(PosicionD, outputD);
  stateMachine.SetOnLeaving(PosicionA, []() {Serial.println("Leaving A"); });
  stateMachine.SetOnLeaving(PosicionB, []() {Serial.println("Leaving B"); });
  stateMachine.SetOnLeaving(PosicionC, []() {Serial.println("Leaving C"); });
  stateMachine.SetOnLeaving(PosicionD, []() {Serial.println("Leaving D"); });
  // Initial state
  stateMachine.SetState(PosicionA, false, true);
}
/**
 * Procedimiento que se encarga de actualizar la maquina de estados <br>
 * <b>post: </b> Procedimiento que se encarga de actualizar el estado
 * de la maquina de estados
 * @see: stateMachine.Update()
 */
// Actualiza el estado de la máquina de estado
void actualizarStateMachine(){
  //captura el estado
  int currentState = stateMachine.GetState();
  switch (currentState)
  {
    case PosicionA: actionA();
                    break;
    case PosicionB: actionB();
                    break;
    case PosicionC: actionC();
                    break;
    case PosicionD: actionD();
                    break;
    default:        Serial.println("state Unknown");
                    break;
  }
  stateMachine.Update();
}

//---------------------------------Control de acceso---------------------------------
/**
 * Procedimiento que se encarga de controlar el acceso <br>
 * <b>post: </b> procedimiento que se encarga de controlar el acceso
 * recibe la clave y valida si es correcta o incorrecta 
 * segun corresponda enciende el led, bloque el sistema
 * despues de 3 claves incorrectas
 * 
 */
void controlAcceso(){
  tiempoFinalKey=millis();
  lcd.setCursor(0,0);
  lcd.print("Ingrese clave:");
  char key = keypad.getKey();
  Serial.print(key);
  if (key){
    tiempoInicioKey=millis();
    contraleida[pos]= key;
    lcd.setCursor(pos,1);
    lcd.print(key);
    Serial.println(key);
    pos++;
    Serial.println(pos);
  }
  if(pos > 3 || (tiempoFinalKey>tiempoInicioKey+5000 && tiempoInicioKey!=0)){
    if(validar() == true){
      lcd.setCursor(0,1);
      lcd.print("Clave correcta");
      lcd.setCursor(0,1);
      analogWrite(RED,0);
      analogWrite(GREEN,255);
      analogWrite(BLUE,0);
      delay(3000);
      lcd.print("                ");
      input=Input::Forward;
      pos=0;
      tiempoInicioKey=tiempoFinalKey=0;
      analogWrite(RED,0);
      analogWrite(GREEN,0);
      analogWrite(BLUE,0);
    return;
    }
    else{
      lcd.setCursor(0,1);
      lcd.print("Clave incorrecta");
      analogWrite(RED,255);
      analogWrite(GREEN,128);
      analogWrite(BLUE,0);
      delay(3000);
      lcd.setCursor(0,1);
      lcd.print("                ");
      intentos++;
      tiempoInicioKey=tiempoFinalKey=0;
    }
    pos = 0;
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
  }
  if(intentos == 3){
    lcd.setCursor(0,1);
    lcd.print("Sistema bloqueado.");
    analogWrite(RED,255);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
    delay(5000);
    lcd.setCursor(0,1);
    lcd.print("                ");
    intentos=0;
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
  }
}
/**
 * Procedimiento que se encarga de validar la clave <br>
 * <b>post: </b> procedimiento que se encarga de validar
 * la clave ingresada
 * @return false
 * @return true
 */
boolean validar(){
  for(int i = 0 ; i<4;i++){
      if(contra[i]!= contraleida[i]) return false;
   }
   return true;
}
//---------------------------------Opciones de Menu---------------------------------
/**
 * Procedimiento que se encarga del control de configuraciones <br>
 * <b>post: </b> Procedimiento que se encarga del control de configuraciones
 * recibiendo un # 
 * @see enterMenu
 */
void controlConfigs(){
  char key = keypad.getKey();
  if(key == '#' && enterMenu == true){
    enterMenu = false;
    lcd.clear();
  }else if(key == '#' && enterMenu == false){
    enterMenu = true;
    lcd.clear();
  }  
}
/**
 * Procedimiento que se encarga de imprimir el menu <br>
 * <b>post: </b> procedimiento que se encarga de imprimir el menu
 * @see: indeceCursor()
 */
void imprimirMenu(){
  lcd.setCursor(0,0);
  lcd.print( frame[page]);
  lcd.setCursor(0,1);
  lcd.print( frame[page+1]);
  indeceCursor();
}
/**
 * Procedimiento que se encarga de la configuracion del menu<br>
 * <b> post: </b> procedimiento que se encarga de la configuracion
 * del menu
 * Para caso 0
 * @see:configTempBaja()
 * Para caso 1
 * @see:configTempAlarma()
 * Para caso 2
 * @see:configLuz()
 * Para caso 3
 * @see:onfigMicro()
 * Para caso 4
 * @see:reiniciar()
 */
void MenuConfig(){
 switch (page){
   case 0:
     if(selectedIndex == 0) configTempAlta();
     else configTempBaja();
     break;
   case 1:
    if(selectedIndex == 0) configTempBaja();
    else configTempAlarma();
    break;
    case 2:
      if(selectedIndex == 0) configTempAlarma();
      else configLuz();
      break;
    case 3:
     if(selectedIndex == 0) configLuz();
     else configMicro();
     break;
   case 4:
     if(selectedIndex == 0) configMicro();
     else reiniciar();
     break;
   case 5:
     if(selectedIndex==0) reiniciar();
     else {
      input=Input::Backward;
      enterMenu=false;
     }
     break;
   case 6:
     if(selectedIndex==0) {
      input=Input::Backward;
      enterMenu=false;}
     else {
      input=Input::Forward;
      enterMenu=false;}
     break;
   default: break;
  }
}
/**
 * Procedimiento controla Menu <br>
 * <b>post: </b> procedimiento que se encarga del control de menu
 * introduciendo '0', '*', '#'
 * @see:controlIndice()
 * @see: controlPagina()
 * @see:imprimirMenu()
 * @menuConfig()
 */
void controlMenu(){
 char key = keypad.getKey();
 
  if (key == '0'){
  lcd.clear();
  selectedIndex++;
  }else if(key == '*'){
  lcd.clear();
  selectedIndex--;
  }else if(key == '#' && enterMenu == false){
  enterMenu = true;
  lcd.clear();
  }
 controlIndice();
 controlPagina();
 imprimirMenu();
 while(enterMenu){
  Serial.println(millis()/1000);
  MenuConfig();
 }
}
//---------------------------------Manejo de lcd---------------------------------
/**
 * Procedimiento que muestra un cursor <br>
 * <b>post:</b> procedimiento que se encarga de que 
 * salga un cursor indicador en el lcd
 */
void indeceCursor(){
 //selectedIndex == 0 ? lcd.setCursor(0,0) : lcd.setCursor(0,1);
 if(selectedIndex == 0) lcd.setCursor(14,0);
 else if(selectedIndex == 1) lcd.setCursor(14,1);
 lcd.print("<-");
}
/**
 * Procedimiento que controla el indice <br>
 * <b>post: </b> procedimiento que se encarga de controlar el indice
 */
void controlIndice(){ // controlar el indice
 if(selectedIndex == 2){
 selectedIndex = 1;
 page++;
 }
 if(selectedIndex < 0){
 page--;
 selectedIndex = 0;
 }
}

//Controla la pagina
/**
 * Procedimiento que controla la pagina <br>
 * <b> post: </b> procedimiento que se encarga de controla la pagina
 */
void controlPagina(){
 if(page == lastFrame ){
 lcd.clear();
 selectedIndex = 0;
 page = 0;
 }else if(page < 0){
 lcd.clear();
 selectedIndex = 1;
 page = lastFrame - 1;
 }
}
//---------------------------------Control de tareas---------------------------------
/**
 * Procedimiento que actualiza las tareas <br>
 * <b>post: </b> procedimiento que se encarga de actualizar
 * las tareas
 * @see:isDownKey()
 */
void actualizarTareas(){
  asyncTemperatura.Update();
  asyncLuz.Update();
  if(isDownKey()){
    Serial.println("downKey");
    input=Input::Backward; //volver a la configuracion
  }
  if(input!=3){// si el estado cambia, las tareas se finalizan
    asyncTemperatura.Stop();
    asyncLuz.Stop();
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
  }
}
/**
 * Procedimiento que muestra la tecla abajo <br>
 * <b>post: </b> procedimiento que se encarga de cambiar de tecla
 * @return
 */
boolean isDownKey(){
  char key=keypad.getKey();
  if(key){
    if(key=='#'){
      if(!down){
        tiempoInicioKey=millis();//Cambiar para que se cumpla
        down=true;
      }else{
        tiempoFinalKey=millis();
      }
    }else{
      tiempoInicioKey=tiempoFinalKey=0;
      down=false;
    }
    if(tiempoFinalKey>tiempoInicioKey+5000 && tiempoInicioKey!=0){
      tiempoInicioKey=tiempoFinalKey=0;
      down=false;
      return true;
    }
    return false;
  }
  else{
      tiempoInicioKey=tiempoFinalKey=0;
      down=false;
      return false;
  }
}
//---------------------------------Calculos de temperatura, luz, microfono---------------------------------
//Funcion para la temperatura
/**
 * Procedimiento que lee la temperatura <br>
 * <b>post: </b> procedimiento que se encarga de leer la temperatura
 */
void leerTemp(){
  long a = 1023 - analogRead(tempPin);
  float tempC = beta /(log((1025.0 * 10 / a - 10) / 10) + beta / 298.0) - 273.0;
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(11, 0);
  lcd.print("    ");
  lcd.setCursor(11, 0);
  lcd.print(tempC);
  Serial.print(millis()/1000);
  Serial.print(", read temp: "); //1,read temp 24 C
  Serial.println(tempC);
  //TODO: Temperatura de alarma
  if(tempC>tempAlarma){
    Serial.println("Sonar la alarma");
    input=Input::Forward; //Hace sonar la alarma
  }else if(tempC>tempAlta){
    //turn red
    analogWrite(RED,250);
    analogWrite(GREEN,0);
    analogWrite(BLUE,0);
  }else if(tempC<tempBaja){
    //turn blue
    analogWrite(RED,0);
    analogWrite(GREEN,0);
    analogWrite(BLUE,250);
  }else{
    //turn green
    analogWrite(RED,0);
    analogWrite(GREEN,250);
    analogWrite(BLUE,0);
  }
}

//Funcion para  la luz
/**
 * Procedimiento funcion para la luz <br>
 * <b> post: </b> procedimiento que se encarga del
 * funcionamiento de la luz
 */
void luz(){
  outputValue = analogRead(fotoPin);
  lcd.setCursor(0, 1);
  lcd.print("Photocell: ");
  lcd.setCursor(11, 1);
  lcd.print("    ");
  lcd.setCursor(11, 1);
  lcd.print(outputValue);//print the temperature on lcd1602
  Serial.print(millis()/1000);
  Serial.print(", read light: "); //1,read temp 24 C
  Serial.println(outputValue);
  if(outputValue>luz){
    analogWrite(RED,128);
    analogWrite(GREEN,0);
    analogWrite(BLUE,128);
  }else{
    analogWrite(RED,255);
    analogWrite(GREEN,233);
    analogWrite(BLUE,0);
  }
}

//Funcion para el microfono
/**
 * Procedimiento funcion para el microfono <br>
 * <b> post: </b> procedimiento que se encarga del
 * funcionamiento del microfono
 */
void microfono(){
 int value = analogRead(microPin);
 Serial.println(value);
 if(value > micro)
 {
 Serial.println("AHH");
 //digitalWrite(ledPin,HIGH);
 //delay(100);
 }
 else
 {
 //digitalWrite(ledPin,LOW);
 }
 delay(100);
}

//---------------------------------Configuraciones de temperatura, luz y microfono---------------------------------
/**
 * Procedimiento que configura la temperatura alta  <br>
 * <b> post: </b> procedimiento que se encarga de
 * la configuracion de la temperatura alta
 */
void configTempAlta(){
  lcd.setCursor(0,0);
  lcd.print("Temp Alta       ");
  char key = keypad.getKey();
  if(key){
    if (key == '0' && tempAlta < 110){
     tempAlta = tempAlta + 1;
    }else if(key == '*' && tempAlta > tempBaja){
     tempAlta = tempAlta - 1;
    }else if(key == '#' && enterMenu == true){
      EEPROM.put(DIR_TEMP_ALTA, tempAlta);
       enterMenu = false;
    }
  }
  lcd.setCursor(0,1); //Imprima en la segunda fila
  lcd.print("Valor: ");
 lcd.setCursor(7,1);
 lcd.print("    ");
 lcd.setCursor(7,1);
 lcd.print(tempAlta);
}
/**
 * Procedimiento que configura la temperatura baja  <br>
 * <b> post: </b> procedimiento que se encarga de
 * la configuracion de la temperatura baja
 */
void configTempBaja(){
  lcd.setCursor(0,0);
  lcd.print("Temp baja       ");
  char key = keypad.getKey();
  if(key){
    if (key == '0' && tempBaja < tempAlta){
    tempBaja = tempBaja + 1;
    }else if(key == '*' && tempBaja > 0){
    tempBaja = tempBaja - 1;
    }else if(key == '#' && enterMenu == true){
      EEPROM.put(DIR_TEMP_BAJA, tempBaja);
      enterMenu = false;
    }
  }
  lcd.setCursor(0,1);
  lcd.print("Valor: ");
 lcd.setCursor(7,1);
 lcd.print("    ");
 lcd.setCursor(7,1);
 lcd.print(tempBaja);
}
/**
 * Procedimiento que configura la alarma  <br>
 * <b> post: </b> procedimiento que se encarga de
 * la configuracion de la alarma
 */
void configTempAlarma(){
  lcd.setCursor(0,0);
  lcd.print("Temp alarma    ");
  char key = keypad.getKey();
  if(key){
    if (key == '0' && tempAlarma < 110){
    tempAlarma = tempAlarma + 1;
    }else if(key == '*' && tempAlarma > tempAlta){
    tempAlarma = tempAlarma - 1;
    }else if(key == '#' && enterMenu == true){
      EEPROM.put(DIR_TEMP_ALARMA, tempAlarma);
      enterMenu = false;
    }
  }
  lcd.setCursor(0,1);
  lcd.print("Valor: ");
  lcd.setCursor(7,1);
 lcd.print("    ");
 lcd.setCursor(7,1);
 lcd.print(tempAlarma);
}
/**
 * Procedimiento que configura la luz  <br>
 * <b> post: </b> procedimiento que se encarga de
 * la configuracion de la luz
 */
void configLuz(){
  lcd.setCursor(0,0);
  lcd.print("Luz             ");
  char key = keypad.getKey();
  if(key){
    if (key == '0' && intensidad < 1023){
    intensidad = intensidad + 1;
    }else if(key == '*' && intensidad > 0){
    intensidad = intensidad - 1;
    }else if(key == '#' && enterMenu == true){
      EEPROM.put(DIR_LUZ, intensidad);
      enterMenu = false;
    }
 }
 lcd.setCursor(0,1);
 lcd.print("Valor: ");
 lcd.setCursor(7,1);
 lcd.print("    ");
 lcd.setCursor(7,1);
 lcd.print(intensidad);
}
/**
 * Procedimiento que configura el microfono <br>
 * <b> post: </b> procedimiento que se encarga de
 * la configuracion del microfono
 */
void configMicro(){
  lcd.setCursor(0,0);
  lcd.print("Micro           ");
  char key = keypad.getKey();
  if(key){
    if (key == '0' && micro < 1023){
    micro = micro + 1;
    }else if(key == '*' && micro > 0){
    micro = micro - 1;
    }else if(key == '#' && enterMenu == true){
      EEPROM.put(DIR_MICRO, micro);
      enterMenu = false;
    }
 }
 lcd.setCursor(0,1);
 lcd.print("Valor: ");
 lcd.setCursor(7,1);
 lcd.print("    ");
 lcd.setCursor(7,1);
 lcd.print(micro);
}
/**
 * Procedimiento que reinicia  <br>
 * <b> post: </b> procedimiento que se encarga de
 * reiniciar
 */
void reiniciar(){
  lcd.clear();
  tempAlta=25;
  tempBaja=15;
  tempAlarma=40;
  intensidad=850;
  micro=30;
  EEPROM.put(DIR_TEMP_ALTA, tempAlta);
  EEPROM.put(DIR_TEMP_BAJA, tempBaja);
  EEPROM.put(DIR_TEMP_ALARMA, tempAlarma);
  EEPROM.put(DIR_LUZ, intensidad);
  EEPROM.put(DIR_MICRO, micro);
  lcd.setCursor(0,1);
  lcd.print("Reinicio exitoso");
  delay(2000);
  lcd.clear();
  enterMenu=false;
}
/**
 * Procedimiento que carga por defecto  <br>
 * <b> post: </b> procedimiento que se encarga de
 * cargar por defecto
 */
void cargarPorDefecto(){
  EEPROM.get(DIR_TEMP_ALTA, tempAlta);
  EEPROM.get(DIR_TEMP_BAJA, tempBaja);
  EEPROM.get(DIR_TEMP_ALARMA, tempAlarma);
  EEPROM.get(DIR_LUZ, intensidad);
  EEPROM.get(DIR_MICRO, micro);
}

//---------------------------------Alarma---------------------------------
/**
 * Procedimiento que activa la alarma  <br>
 * <b> post: </b> procedimiento que se encarga hacer
 * sonar la alarma
 */
void alarma(){
  lcd.setCursor(1,1);
  lcd.print("Alarma sonando");
  tone(bzz,FRECUENCIA,TIEMPO_SONIDO);
  delay(TIEMPO_SONIDO+50);
  noTone(bzz);
  input=Input::Forward;
}
