create table message(
    channel smallint not null ,
    seq     int unsigned not null,
    time    DATE,
    content varchar(5000) not null,
    primary key(channel,seq)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;
