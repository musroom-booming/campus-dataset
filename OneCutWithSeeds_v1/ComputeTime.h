


#pragma once




/*




//������������ʱ�����




//




*/




#include <iostream>









#ifndef ComputeTime_h




#define ComputeTime_h














class   ComputeTime




{




private:




	int Initialized;




	__int64 Frequency;




	__int64 BeginTime;









public:









	bool Avaliable();




	double End();




	bool Begin();




	ComputeTime();




	virtual   ~ComputeTime();









};


































#endif
