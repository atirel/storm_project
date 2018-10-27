# storm_project

This LLVM Pass was written during an internship at Inria Bordeaux sud ouest (Talence France).
As far as my knowledge goes for copyrights, it is available for everyone and can be seen by anyone.

Version française : ligne 12
English version: line 55
Versión Española: linea 



Ce répertoire présente quatre passes LLVM (http://llvm.org/docs/WritingAnLLVMPass.html (lien anglophone)) qui représentent différentes approches pour régler le même problème à savoir :

Insérer manuellement des opérations permettant de mettre à 0 toutes les variables avant leur initialisation, après leur dernière utilisation en fin de programme et après leur dernière utilisation "utile" (après la dernière lecture de leur valeur et avant la prochaine écriture de leur valeur).

Actuellement, la passe Initialize permet d'insérer des instructions pour "pré-initialiser" toutes les variables en leur affectant la valeur zéro (null pour les pointeurs).

La passe PutAtZero réalise en plus de cette initialisation une mise à zéro des variables après leur dernière utilisation mais n'arrive pas encore à détecter le cas particulier où des variables existantes sont ensuite référencées par des pointeurs qui sont ensuite eux-mêmes utilisés.
Cette passe utilise une approche par graphe du programme.

Les Passes DoubleStoreElimination et DeadVariableHandler sont d'anciennes approches qui n'ont pas abouti et qui servent surtout à prendre la main sur LLVM


Notes :

*La version de LLVM minimale requise est la version 3.9
*Les détails de la compilation de LLVM et de la réalisation d'une passe sont disponibles sur le site de LLVM (version française en cours de rédaction de mon côté)
=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+

   |:~8a.`~888a:::::::::::::::88......88:::::::::::::::;a8~".a888~|     |* * * * * * * * * * OOOOOOOOOOOOOOOOOOOOOOOOO|
   |::::~8a.`~888a::::::::::::88......88::::::::::::;a8~".a888~:::|     | * * * * * * * * *  OOOOOOOOOOOOOOOOOOOOOOOOO|
   |:::::::~8a.`~888a:::::::::88......88:::::::::;a8~".a888~::::::|     |* * * * * * * * * * OOOOOOOOOOOOOOOOOOOOOOOOO|
   |::::::::::~8a.`~888a::::::88......88::::::;a8~".a888~:::::::::|     | * * * * * * * * *  OOOOOOOOOOOOOOOOOOOOOOOOO|
   |:::::::::::::~8a.`~888a:::88......88:::;a8~".a888~::::::::::::|     |* * * * * * * * * * OOOOOOOOOOOOOOOOOOOOOOOOO|
   |::::::::::::::::~8a.`~888a88......88;a8~".a888~:::::::::::::::|     |* * * * * * * * * * OOOOOOOOOOOOOOOOOOOOOOOOO|
   |:::::::::::::::::::~8a.`~888......88~".a888~::::::::::::::::::|     | * * * * * * * * *  OOOOOOOOOOOOOOOOOOOOOOOOO|
   |8888888888888888888888888888......8888888888888888888888888888|     |* * * * * * * * * * OOOOOOOOOOOOOOOOOOOOOOOOO|
   |..............................................................|     |OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO|
   |..............................................................|     |OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO|
   |8888888888888888888888888888......8888888888888888888888888888|     |OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO|
   |::::::::::::::::::a888~".a88......888a."~8;:::::::::::::::::::|     |OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO|
   |:::::::::::::::a888~".a8~:88......88~888a."~8;::::::::::::::::|     |OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO|
   |::::::::::::a888~".a8~::::88......88:::~888a."~8;:::::::::::::|     |OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO|
   |:::::::::a888~".a8~:::::::88......88::::::~888a."~8;::::::::::| 
   |::::::a888~".a8~::::::::::88......88:::::::::~888a."~8;:::::::|
   |:::a888~".a8~:::::::::::::88......88::::::::::::~888a."~8;::::|
   |a888~".a8~::::::::::::::::88......88:::::::::::::::~888a."~8;:|
                                    
=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+
Flag credits:
-Wayne Jones, Coventry UK- (UK Flag)

=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+=+=+=+=+~~~~+=+==+===+

English version:

Versión Española:
