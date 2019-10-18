#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../apmib/apmib.h"
#include "../src/deviceProcIf.h"

static struct ifstatRate wanRate;

int main(void)
{
	char *data;
	time_t current;
	char buffer[512];
	int upspeed = 222;
	int downspeed = 1067;
	char rxRate[32] = {0};
	char txRate[32] = {0};
	
	printf("content-type:text/html\n\n");  

	wanRate.ifname="eth1";
	getProcIfData(&wanRate);

	//printf("wanRate rxRate = %.2lf\n", wanRate.rxRate);
	//printf("wanRate txRate = %.2lf\n", wanRate.txRate);
	if (wanRate.txRate < 1000)
		sprintf(txRate, "%.2lfKbps", wanRate.txRate);
	else
		sprintf(txRate, "%.2lfMbps", wanRate.txRate/1000);

	if (wanRate.rxRate < 1000)
		sprintf(rxRate, "%.2lfKbps", wanRate.rxRate);
	else
		sprintf(rxRate, "%.2lfMbps", wanRate.rxRate/1000);
	
	data=getenv("QUERY_STRING");  
	if(data==NULL)    
	{
		printf("ret:upspeed=%sTX&downspeed=%sRX");  
	}
	else
	{
		printf("ret:upspeed=%sTX&downspeed=%sRX", txRate, rxRate);
	}
	
	return 0;
}


