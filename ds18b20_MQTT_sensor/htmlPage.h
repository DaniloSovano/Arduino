const char serverIndex[] = R"(
    <!DOCTYPE HTML>
    <html lang='pt-BR'>
    <head>
        <meta charset='UTF-8'>
        <title>Atualização de Firmware</title>
        <style>
            body {
                background-color: #111111;
                font-family: Arial, sans-serif;
                text-align: center;
                padding: 60px;
            }
            h1 {
                color: #9966CB;
                font-size: 50px;
            }
            h2 {
                color: white;
                font-size: 30px;
            }
            p {
                font-size: 45px;
                color: #cccccc;
                font-weight: bold;
            }
            span {
                font-size: 40px;
                color: #cccccc;
            }
            input[type='file'] {
                margin-top: 20px;
                font-size: 18px;
                color: white;
            }
            input[type='submit'], input[type='button'] {
                background-color: #9966CB;
                color: white;
                border: none;
                padding: 10px 20px;
                font-size: 18px;
                cursor: pointer;
                border-radius: 5px;
                margin-top: 10px;
            }
            input[type='submit']:hover {
                background-color: #7748A6; 
            }
            form[action='/restart'] input[type='submit'] {
                background-color: #cc3333;
            }
            form[action='/restart'] input[type='submit']:hover {
                background-color: #993333;
            }
        </style>
        <script type='text/javascript'>
            function updateMessage() {
                document.getElementById('status').innerHTML = 'Tempo limite de OTA atingido. Reiniciando o ESP...';
            }

            function validateForm() {
                const fileInput = document.querySelector("input[type='file']");
                if (!fileInput.files.length) {
                    alert("Por favor, selecione um arquivo para atualizar.");
                    return false;
                }
                return true;
            }
        </script>
    </head>
    <body>
        <h1>Atualização de Firmware</h1>
        <h2>Escolha um arquivo .bin</h2>
        <form method='POST' action='/update' enctype='multipart/form-data' onsubmit='return validateForm()'>
            <input type='file' name='update'>
            <input type='submit' value='Atualizar Firmware'>
        </form>

        <form method='POST' action='/restart' onsubmit='return confirm("Você realmente deseja sair? O ESP será reiniciado.")'>
            <input type='submit' value='Sair'>
        </form>

        <h2 id="status"></h2>

        <script type="text/javascript">
            setTimeout(updateMessage, 180000);  
        </script>
    </body>
    </html>
)";
