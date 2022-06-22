# cpp-advanced-vector

Аналог шаблонного класса vector из стандартной библиотеки.
Данные хранятся в памяти, динамически выделяемой в куче. Выделяется неинициализированная память, инициализация происходит при фактическом добавлении элементов в вектор. Если имеющейся памяти недостаточно - выделяется новый участок памяти размером в два раза больше предыдущего, в который перемещаются (либо копируются) данные из старого участка , после чего старый участок освобождается.
