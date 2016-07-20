# EV_HTTPCLIENT
Асинхронный HTTP клиент (crawler) с использованием библиотеки [libev] и [PicoHTTPParser]. 

```bash
Usage: ev_httpclient [KEY]... IP-LIST

	-n	number asynchronous requests
	-o  output file found domains

Example:
$ ev_httpclient -n 50 -o found.csv domains.dat
```

В данной реализации IP-LIST - это бинарный файл генерирумый другой программой [DNS resolver]. 
Формат файла:
```
Заголовок: 
4 байта MAGIC 0xDEADBEEF
4 байта общий размер данных в записи

Данные:
4 байта размер данных
n байт данных
```

Содержание файла:
```
Заголовок
имя домена без завершающего нуля
ip адрес домена из структуры hostent 
```

Если вы хотите запустить процесс, как демон не привязанный к терминалу:
```bash
$ setsid cmd >/dev/null 2>&1
```
##Installation
```bash
$ make
```

### libev
Библиотека для упрощения и унификации поддержки механизма асинхронного неблокирующего ввода/вывода и механизма оповещения о событиях с помощью выполнения обратного вызова (callback) функции при наступлении заданного события для дескриптора файла или при достижении заданного таймаута (timeout). 

### PicoHTTPParser
Простой, примитивный, быстрый парсер HTTP запросов/ответов.

[libev]: http://software.schmorp.de/pkg/libev.html
[PicoHTTPParser]: https://github.com/h2o/picohttpparser
[DNS resolver]:https://github.com/Garik-/dns_resolver