#include <string.h>
#include "redis.h"
#include "util.h"

int yesnotoi(char *s) {
    if (!strcasecmp(s,"yes")) return 1;
    else if (!strcasecmp(s,"no")) return 0;
    else return -1;
}

/**
 * 从文件和option中载入配置
 */ 
void loadServerConfig(char *filename, char *option){

    sds config = sdsempty();
    char buf[REDIS_CONFIGLINE_MAX + 1];

    if(filename){

    }

    if(option){
        
    }
}

void loadServerConfigFromString(char *config){
    sds *argv;  //参数数组，保存每一行的各个参数名和值，注意一行可能有多个配置
    int lineNum = 0;
    int totalLines;
    sds *lines; //数组，每一个元素都是一项配置字符串
    lines = sdssplitlen(config, strlen(config), "\n", 1, &totalLines);
    int i;
    char *err = NULL;
    for (i = 0; i < totalLines; i++){
        lineNum = i + 1;
        //移除当前行头尾的空白
        lines[i] = sdstrim(lines[i], " \t\r\n");
        //如果是注释或者空白行，直接跳过
        if(lines[i][0] == '#' || lines[i][0] == '\0'){
            continue;
        }
        int argc;
        //将字符串分割成多个参数
        argv = sdssplitargs(lines[i], &argc);
        if(argv == NULL){
            err = "Unbalanced quotes in configuration line";
            goto loaderr;
        }

        //跳过空白参数
        if(argc == 0){
            sdsfreesplitres(argv, argc);
            continue;
        }

        //将配置项转成小写，不明白为什么只转换第一项
        sdstolower(argv[0]);

        //开始检查参数值是否合理，并开始处理配置参数，每次只会满足1个条件
        if(!strcasecmp(argv[0], "timeout") && argc == 2){
            server.maxidletime = atoi(argv[0]);
            if(server.maxidletime < 0){
                err = "Invalid timeout value";
                goto loaderr;
            }
        }else if(!strcasecmp(argv[0], "tcp-keepalive") && argc == 2){
            server.tcpkeepalive = atoi(argv[0]);
            if(server.tcpkeepalive < 0){
                err = "Invalid tcp-keepalive value";
                goto loaderr;
            }
        }else if(!strcasecmp(argv[0], "port") && argc == 2){
            server.port = atoi(argv[0]);
            if(server.port < 0 || server.port > 65535){
                err = "Invalid port value";
                goto loaderr;
            }
        }else if(!strcasecmp(argv[0], "tcp-backlog") && argc == 2){
            server.tcp_backlog = atoi(argv[0]);
            if(server.tcp_backlog < 0){
                err = "Invalid tcp-backlog value";
                goto loaderr;
            }
        }else if(!strcasecmp(argv[0], "logfile") && argc == 2){
            //要先free默认值
            free(server.logfile);
            server.logfile = strdup(argv[1]);
            //如果文件名不为空，则尝试打开一次，检查文件本身是否存在
            if(server.logfile[0] != '\0'){
                FILE *fp = fopen(server.logfile, "a");
                if(fp == NULL){
                    err = "Can't open the logfile";
                    goto loaderr;
                }
                fclose(fp);
            }
        }else if(!strcasecmp(argv[0], "databases") && argc == 2){
            server.dbnum = atoi(argv[0]);
            if(server.dbnum < 1){
                err = "Invalid number of databases";
                goto loaderr;
            }
        }else if(!strcasecmp(argv[0], "maxmemory") && argc == 2){
            server.maxmemory = memtoll(argv[1], NULL);
        }else if(!strcasecmp(argv[0], "daemonize") && argc == 2){
            if((server.daemonize = yesnotoi(argv[1]))==-1){
                err = "argument must be 'yes' or 'no'";
                goto loaderr;
            }
        }else if(!strcasecmp(argv[0], "hz") && argc == 2){
            server.hz = atoi(argv[1]);
            if(server.hz < REDIS_MIN_HZ){
                server.hz = REDIS_MIN_HZ;
            }else if(server.hz > REDIS_MAX_HZ){
                server.hz = REDIS_MAX_HZ;
            }
        }else if(!strcasecmp(argv[0], "pidfile") && argc == 2){
            //先释放后复制新值
            free(server.pidfile);
            server.pidfile = strdup(argv[1]);
        }else{
            err = "Bad directive or Wrong number of arguments";
            goto loaderr;
        }
        //最终要把解析出来的参数数组free
        sdsfreesplitres(argv, argc);
    }

    //整行版本的配置数组也得释放
    sdsfreesplitres(lines, totalLines);

    return;

    loaderr:
        fprintf(stderr, "\n***FATAL CONFIG FILE ERROR\n");
        fprintf(stderr, "Reading the configuration file, at line %d\n", lineNum);
        fprintf(stderr, ">>> '%s'\n", lines[i]);
        fprintf(stderr, "%s\n", err);
        exit(1);
}