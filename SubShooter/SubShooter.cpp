// SubShooter.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "ShooterAPI.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("\nUsage: SubShooter.exe video-filename\n");
		return 0;
	}
	CShooterAPI shooterAPI;
	if (shooterAPI.GetFirstSubTitle(argv[1]) == false) {
		printf(shooterAPI.GetError());
	}
	return 0;

}

