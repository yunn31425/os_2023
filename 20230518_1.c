#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <aio.h>

#define BSZ 4096
#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

unsigned char buf[BSZ];

unsigned char shift(unsigned char c);

int main(int argc, char* argv[])
{
	int	ifd, ofd, i, n, nw, cur;

	struct aiocb control;

	bzero( (char *) &control, sizeof(struct aiocb));
    // aio 컨트롤 블록 초기화

	if (argc != 3){
		printf("usage: rot13 infile outfile\n");
		strerror(errno);
	}
        
    // 입력 파일을 읽기 전용으로 열기	
	if ((ifd = open(argv[1], O_RDONLY)) < 0){
		printf("can't open %s\n", argv[1]);
		strerror(errno);
	}

	// 출력 파일을 생성
	if ((ofd = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, FILE_MODE)) < 0){
		printf("can't create %s\n", argv[2]);
 		strerror(errno);
	}
	
    // aiocb 설정
	control.aio_fildes = ifd;
	control.aio_offset = 0;
	control.aio_buf = (char*) malloc(BSZ+1);
	control.aio_nbytes = BSZ;

    // 비동기 읽기 시작
    aio_read(&control);

    // 읽기가 끝날때까지 대기
	while (aio_error(&control) == EINPROGRESS){
		printf("reading!\n");
	}
	
    //입력 파일 크기를 BSZ 크기만큼 읽기
    if ((n = aio_return(&control)) > 0) {
    	// shift함수로 입력한 글자를 판단하고 암호화
    	for (i = 0; i < n; i++){
            buf[i] = shift(((char*) control.aio_buf)[i]);
        }


        // 컨트롤 블록의 fd를 쓰기 fd로 변경
        control.aio_fildes = ofd;
        // 컨트롤 블록의 버퍼를 shift 완료한 값으로 변경
        control.aio_buf = buf;
        // 쓸 길이를 읽기 후 aio_return의 길이로 변경
        control.aio_nbytes = n;

        // 비동기 쓰기 시작
        aio_write(&control);

        // 비동기 쓰기 완료시까지 대기 
        while (aio_error(&control) == EINPROGRESS){
            printf("writing!\n");
        }

    	// 출력 파일에 기록하기 
    	if ((nw = aio_return(&control)) != n) {
    		// 쓰기 요청이 실패한 경우
    		if (nw < 0){
    			printf("write failed\n");
    			strerror(errno);
    		}
    		else{ // 요청한 쓰기 길이보다 적게 쓴 경우
    			printf("parial write (%d/%d)\n", nw, n);
    			strerror(errno);
    		}
    	}
    }

	// 쓰기 결과를 버퍼에서 디스크로 완전히 내리기
	fsync(ofd);
	exit(0);
}

unsigned char shift(unsigned char c)
{
	// 문자열일 때 n/N보다 크면 13자리 아래로
	// n/N 보다 적으면 13자리 위로
	if (isalpha(c)) {
		if (c >= 'n')
			c -= 13;
		else if (c >= 'a')
			c += 13;
		else if (c >= 'N')
			c -= 13;
		else
			c += 13;
	}
	return(c);
}


