# Protokoll: Betriebssysteme & Netzwerke

Übung: Task Ventilator Projekt

Datum: 10. April 2025

Gruppe: 1

Teilnehmer: Katerina Divacky

Matrikel-Nr.: 52308232
 
# Inhaltsverzeichnis
- Welche Themen wurden adressiert?

- Was habe ich gelernt?	

- Welche Kenntnisse habe ich gewonnen?	

- Wie kann ich das erlangte Wissen im Assignment anwenden?	

- Was musste ich für die Lösung zusätzlich erarbeiten?	

- Welche interessanten Aspekte gab es in der Umsetzung?	

- Was waren die Schwierigkeiten?	

- Quelle	

# Welche Themen wurden adressiert?
Hinter dieser Übung versteckt sich die Inter-Process-Communication mithilfe der POSIX Message Queues. Dabei wird ein sogenannter Task Ventilator realisiert: Dieser verteilt an Worker-Prozessen die Tasks, die von den Workern simuliert ausgeführt werden. In dieser Übung sollte also eine Prozessverwaltung mithilfe von fork() realisiert werden; weiterhin sollte die Kommunikation mithilfe von Message Queues zwischen den Prozessen und die Synchronisation mithilfe des semaphorischen Systems erreicht werden. Außerdem sollten die von den Prozessen genutzten Ressourcen beendet und freigegeben werden. Am Ende zusammengenommen ergab sich die Lösung, die unabhängig von der Anzahl von Workern, derer Anzahl von übergebenen und der Queues-Größe ist.

# Was habe ich gelernt?
Ich habe gelernt, wie man mit POSIX Message Queues Prozesse effizient miteinander kommunizieren lassen kann, ohne auf Shared Memory oder Pipes zurückzugreifen. Außerdem habe ich besser verstanden, wie sich blockierende Operationen verhalten und wie man sie durch sauberes Fehlerhandling (z. B. EINTR behandeln) absichert. Besonders spannend war auch zu sehen, wie Prozesse gleichzeitig arbeiten, ohne sich gegenseitig zu beeinflussen – und dass man trotzdem die Übersicht behalten muss, welche Nachrichten wo ankommen. Ein weiteres Learning war die Vermeidung von Race Conditions durch gezieltes Blockieren bei mq_send, wenn die Queue voll ist.

# Welche Kenntnisse habe ich gewonnen?
Während der Umsetzung der Aufgabe konnte ich viele praktische Fähigkeiten in der Prozesskommunikation und -verwaltung sammeln. Die Erstellung von neuen Workerprozessen in der Hauptprozessfunktion mithilfe der Methode fork() und die Priorität der Aufgabe zum Beenden nach dem Start mithilfe von waitpid() war sehr wichtig. Ein weiteres Theme war die Arbeit mit POSIX-Message Queues, mit dem ich mich zuvor nicht befasst hatte. Da ich in diesem Programm lernen konnte, wie man mit mq_open() eine neue Queue öffnet oder erstellt, mit mq_send() Nachrichten versendet und mit mq_receive() Nachrichten wieder ausliest, sehe ich die Arbeit schon als erfolgreich an. Wichtig ist natürlich auch, dass man nach der Nutzung die Queues mit mq_close() schließt und mit mq_unlink() sauber entfernt, damit eine Reste übrigbleiben, wenn man das nächste Mal das Programm startet. Besonders spannend war es meiner Meinung, die Besonderheiten von blockierenden I/O-Operationen zu erfahren. Wenn z.B. die Queue voll ist, wird ein mq_send() blockiert, bis wieder Platz verfügbar ist. Wenn andererseits keine neue Nachricht vorliegt, blockiert mq_receive(). Hier habe ich gelernt, wie man damit umgeht und das Verhalten korrekt einplant. Ein weiteres wichtiges Thema war die Synchronisation zwischen Prozessen. Obwohl einzelne Prozesse nichts voneinander wussten, mussten sie sich im Grunde dennoch über Queue abstimmen – kein Worker durfte die Terminierungsnachricht vor dem anderen bekommen. Ich habe gelernt, wie man das Verhalten so steuert, dass es trotz allem zuverlässig funktioniert. Schließlich habe ich den Umgang mit Fehlerfällen und robustem Queue-Management unterschieden – auf welcher Ebene behandelt man z.B. den Löberger EINTR, das Signal, das eine blocking Operation unterbricht, und umsetzbarer wird, und wie kann man präzise Wiederholungen beibehalten. Schließlich gelang es mir, die erhaltenen Ergebnisse der Worker-Prozesse korrekt zu empfangen und verwerten, sie übersichtlich in die Ausgabe zu integrieren, ohne dabei den Bezug zu den einzelnen Prozessen zu verlieren, was insbesondere in jenen Fällen nützlich ist, in denen mehrere Worker parallel laufen.

# Wie kann ich das erlangte Wissen im Assignment anwenden?
Die Kenntnisse aus dieser Aufgabe lassen sich gut auf andere Projekte übertragen, in denen mehrere Prozesse Aufgaben parallel bearbeiten und miteinander kommunizieren müssen. Besonders in Bereichen wie Simulationen, verteilten Systemen oder beim Bauen von eigenen Serversystemen ist dieses Wissen extrem wertvoll. Auch für spätere Anwendungen mit Threads oder Netzwerkkommunikation bildet dieses Grundverständnis eine stabile Basis.

# Was musste ich für die Lösung zusätzlich erarbeiten?
Ich habe mir zusätzlich nochmal die Manpages zu allen relevanten POSIX-Funktionen durchgelesen, insbesondere mq_open, mq_send, mq_receive und mq_unlink. Auch die Bedeutung und das richtige Setzen von mq_attr war mir vorher nicht ganz klar. Zusätzlich habe ich einige Beispiele zu blocking behavior von Queues angeschaut und ausprobiert, wie man zuverlässig erkennt, wann eine Nachricht empfangen wurde oder ein Fehler vorliegt. Ein kleiner Exkurs war auch nötig zum Thema Restarting Syscalls, also was man tun muss, wenn eine blocking-Queue durch ein Signal unterbrochen wird.

# Welche interessanten Aspekte gab es in der Umsetzung?
Besonders interessant fand ich, dass man durch die Queue-Größe und die Anzahl der Worker das Verhalten des Systems aktiv steuern und beobachten konnte. Wenn die Queue sehr klein war, hat der Ventilator geblockt, bis wieder Platz war – was für eine natürliche Lastverteilung gesorgt hat. Auch der Moment, in dem die Termination-Nachrichten geschickt wurden, musste genau durchdacht sein, damit alle Worker sauber beenden, aber keine Aufgabe verloren geht. Die Verwendung von zufälligen Schlafzeiten hat außerdem geholfen, das Verhalten realistischer zu simulieren.

# Was waren die Schwierigkeiten?
Eine der größten Herausforderungen war, sicherzustellen, dass alle Tasks exakt einmal verarbeitet werden – und dass kein Worker seine Termination zu früh oder zu spät bekommt. Auch die Synchronisation beim Warten auf die Termination-Messages war nicht ganz trivial. Ich musste aufpassen, dass der Ventilator nicht zu früh schließt oder sich aufhängt, weil er auf Worker wartet, die bereits beendet sind. Außerdem war es nicht ganz einfach, bei mehreren Prozessen den Überblick zu behalten, wer was gerade macht – das Logging mit Zeitstempeln hat mir dabei aber geholfen.

# Quelle
- Lehrveranstaltung Betriebssysteme und Netzwerke


- https://man7.org/linux/man-pages/

-> Verwendet für: Detaillierte Informationen zu POSIX-Funktionen (mq_open, mq_send, mq_receive, mq_unlink, fork, waitpid, etc.)
