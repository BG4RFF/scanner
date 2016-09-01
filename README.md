# scanner
Python script writen by Christian Wuethrich (HB9TXW) to communicate to the antenna analyser according to DG7EAO
# Pour les francophones :-)

Le script nécessite PyPlot et PySerial et probablement quelques autres bricoles qui sont livrées d'office avec Python.
Sous linux, mettre le fichier en mode exécution après téléchargement ($ chmod +x scanner.py)
Le scripte prend par défaut /dev/ttyUSB0 com port série toutefois les paramètres suivant permettent de changer le comportement au démarrage:
-h --help: donne une aide succinte
-p --port [] permet de définir le port com à utiliser
-o --output: permet de définir un fichier dans lequel les données de mesures sont stockées pour utilisation ultérieure sur un tableur par exemple
-v --verbose : affiche à l'écran les valeurs intermédiaires et la communication (débogage)
On peut tout utiliser en même temps du hgenre:
$ ./scanner -v -p com3 -o mon_antenne.txt

Have fun!
73!
