all: HospitalA HospitalB HospitalC Student1 Student2 Student3 Student4 Student5 Healthcenter

HospitalA: HospitalA.cpp
	g++ -o HospitalA HospitalA.cpp

HospitalB: HospitalB.cpp
	g++ -o HospitalB HospitalB.cpp

HospitalC: HospitalC.cpp
	g++ -o HospitalC HospitalC.cpp

Healthcenter: Healthcenter.cpp
	g++ -o Healthcenter Healthcenter.cpp

Student1: Student1.cpp
	g++ -o Student1 Student1.cpp

Student2: Student2.cpp
	g++ -o Student2 Student2.cpp

Student3: Student3.cpp
	g++ -o Student3 Student3.cpp

Student4: Student4.cpp
	g++ -o Student4 Student4.cpp

Student5: Student5.cpp
	g++ -o Student5 Student5.cpp
