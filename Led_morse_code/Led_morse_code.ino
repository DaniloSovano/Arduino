
char palavra[] = "sos";
char *p = palavra;
// Função pra piscar um ponto(.)
void piscarPonto() {
    digitalWrite(D4, HIGH);  // Acende o LED
    delay(120);                   // Espera por 120 ms
    digitalWrite(D4, LOW);   // Apaga o LED
    delay(120);                   // Pausa de 120 ms entre os pontos e traços
}

// Função para piscar um traço (-)
void piscarTraco() {
    digitalWrite(D4, HIGH);  // Acende o LED
    delay(360);                   // Espera por 360 ms
    digitalWrite(D4, LOW);   // Apaga o LED
    delay(120);                   // Pausa de 120 ms entre os pontos e traços
}



void imprimir(char *p ){
    for (int i = 0; p[i] != '\0'; i++){
    switch(p[i]) {

    case 'a':
        piscarPonto();
        piscarTraco();
        break;
    case 'b':
        piscarTraco();
        piscarPonto();
        piscarPonto();
        piscarPonto();
        break;
    case 'c':
        piscarTraco();
        piscarPonto();
        piscarTraco();
        piscarPonto();
        break;
    case 'd':
        piscarTraco();
        piscarPonto();
        piscarPonto();
        break;
    case 'e':
        piscarPonto();
        break;
    case 'f':
        piscarPonto();
        piscarPonto();
        piscarTraco();
        piscarPonto();
        break;
    case 'g':
        piscarTraco();
        piscarTraco();
        piscarPonto();
        break;
    case 'h':
        piscarPonto();
        piscarPonto();
        piscarPonto();
        piscarPonto();
        break;
    case 'i':
        piscarPonto();
        piscarPonto();
        break;
    case 'j':
        piscarPonto();
        piscarTraco();
        piscarTraco();
        piscarTraco();
        break;
    case 'k':
        piscarTraco();
        piscarPonto();
        piscarTraco();
        break;
    case 'l':
        piscarPonto();
        piscarTraco();
        piscarPonto();
        piscarPonto();
        break;
    case 'm':
        piscarTraco();
        piscarTraco();
        break;
    case 'n':
        piscarTraco();
        piscarPonto();
        break;
    case 'o':
        piscarTraco();
        piscarTraco();
        piscarTraco();
        break;
    case 'p':
        piscarPonto();
        piscarTraco();
        piscarTraco();
        piscarPonto();
        break;
    case 'q':
        piscarTraco();
        piscarTraco();
        piscarPonto();
        piscarTraco();
        break;
    case 'r':
        piscarPonto();
        piscarTraco();
        piscarPonto();
        break;
    case 's':
        piscarPonto();
        piscarPonto();
        piscarPonto();
        break;
    case 't':
        piscarTraco();
        break;
    case 'u':
        piscarPonto();
        piscarPonto();
        piscarTraco();
        break;
    case 'v':
        piscarPonto();
        piscarPonto();
        piscarPonto();
        piscarTraco();
        break;
    case 'w':
        piscarPonto();
        piscarTraco();
        piscarTraco();
        break;
    case 'x':
        piscarTraco();
        piscarPonto();
        piscarPonto();
        piscarTraco();
        break;
    case 'y':
        piscarTraco();
        piscarPonto();
        piscarTraco();
        piscarTraco();
        break;
    case 'z':
        piscarTraco();
        piscarTraco();
        piscarPonto();
        piscarPonto();
        break;
    }
    delay(840);
    }
    }
void setup() {
pinMode(D4, OUTPUT);

}

void loop() {
  imprimir(palavra);
  delay(2000);
}
