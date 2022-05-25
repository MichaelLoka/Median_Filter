#include <iostream>
#include <mpi.h>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <list>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}

void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

MPI_Status Status;					// Recived Status
int SubHeight, LastSubHeight;		// Number of Rows for each Processor (And the Last Processor)
int SubSize, LastSubSize;			// Number of Date for each Processor (And the Last Processor)
int Proc_Init_Pos, Proc_End_Pos;	// Range of Data Sent to each Processor
int New_Init_Pos;					// Row Start of each Data Sent to each Processor
int Proc_Data;						// Data Recieved by each Processor

void SendData(int* image, int size, int ImageWidth, int ImageHeight)
{
	for (int i = 1; i < size; i++)
	{
		// Processor Start Position
		Proc_Init_Pos = i * SubSize;
		// Last Processor
		if (i == size - 1)
			SubSize += LastSubSize;
		// New Row Start
		New_Init_Pos = ImageWidth;

		MPI_Send(&SubSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);		// Sending Nubmer of Pixels
		MPI_Send(&ImageHeight, 1, MPI_INT, i, 0, MPI_COMM_WORLD);	// Sending Image Height
		MPI_Send(&ImageWidth, 1, MPI_INT, i, 0, MPI_COMM_WORLD);	// Sending Image Width
		MPI_Send(&New_Init_Pos, 1, MPI_INT, i, 0, MPI_COMM_WORLD);	// Sending the New Start Position

		// Starting Pixel Index for the Last Processor
		int LastInitPos = Proc_Init_Pos - ImageWidth;
		// Last Processor
		if (i == size - 1)
		{
			Proc_Data = SubSize + ImageWidth;							// Next Row Index
			MPI_Send(&Proc_Data, 1, MPI_INT, i, 0, MPI_COMM_WORLD);		// Sending the Processor Data
			MPI_Send(&image[LastInitPos], Proc_Data, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		else
		{
			Proc_Data = SubSize + ImageWidth * 2;						// After Next Row Index
			MPI_Send(&Proc_Data, 1, MPI_INT, i, 0, MPI_COMM_WORLD);		// Sending the Processor Data
			MPI_Send(&image[LastInitPos], Proc_Data, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
	}
}

int* FormMedianArray(int* Image, int ImageWidth, int Pixel_Pos, int Data_Size)
{
	int* values = new int[9];

	//Get 9 values left,right,up,down ,up-right,up-left,down-right,down-left
	// 
	// val[0] val[1] val[2]
	// val[3] val[4] val[5]
	// val[6] val[7] val[8]
	// 
	// if the point postion /width == 0 then the pixel is on the top
	// if the point postion /width == width-1 then the pixel is on the down
	// if the point postion % width == 0 then the pixel is on the left
	// if the point postion % width == width-1 then the pixel is on the right
	
	values[4] = Image[Pixel_Pos];
	values[1] = Pixel_Pos - ImageWidth >= 0 ? Image[Pixel_Pos - ImageWidth] : values[4];
	values[7] = Pixel_Pos + ImageWidth < Data_Size ? Image[Pixel_Pos + ImageWidth] : values[4];

	if (Pixel_Pos % ImageWidth == 0)	
	{
		values[3] = values[4];
		values[0] = values[4];
		values[6] = values[7];
	}
	else
	{
		values[3] = Image[Pixel_Pos - 1];
		if (Pixel_Pos - 1 - ImageWidth < 0) 
			values[0] = values[4]; 
		else
			values[0] = Image[Pixel_Pos - 1 - ImageWidth]; 
		if (Pixel_Pos - 1 + ImageWidth >= Data_Size) 
			values[6] = values[4]; 
		else
			values[6] = Image[Pixel_Pos - 1 + ImageWidth]; 
	}

	if ((Pixel_Pos + 1) % ImageWidth == 0)	
	{
		values[5] = values[4];
		values[2] = values[4];
		values[8] = values[7];
	}
	else
	{
		values[5] = Image[Pixel_Pos + 1]; 
		if (Pixel_Pos + 1 - ImageWidth < 0) 
			values[2] = values[4]; 
		else
			values[2] = Image[Pixel_Pos + 1 - ImageWidth]; 
		if (Pixel_Pos + 1 + ImageWidth >= Data_Size) 
			values[8] = values[4]; 
		else
			values[8] = Image[Pixel_Pos + 1 + ImageWidth]; 
	}
	return values;
}

int* ApplyFilter(int* Sub_Image,int ImageWidth,int Sub_Size_Filtered)
{
	int* afterfilter = new int[Sub_Size_Filtered];
	int* values = new int[9];
	int counter = 0;
	for (int i = New_Init_Pos; i < Proc_End_Pos; i++)
	{
		values = FormMedianArray(Sub_Image, ImageWidth, i, Proc_Data);


		sort(values, values + 9);
		afterfilter[counter++] = values[4];
	}
	
	return afterfilter;
}

int main(int args, char** argv)
{
	MPI_Init(NULL, NULL);
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0;

	//Getting the # of Processors
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	//Getting the Rank of Every Processor
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// The Original Image
	int* Image = NULL;
	// Sub Array Image for each Processor
	int* Sub_Image = NULL;
	// Sub Array Image for each Processor (After Filter)
	int* Sub_Image_Filtered;
	int MasterProc = 0;

	// Master Processor (Initial)
	if (rank == MasterProc)
	{
		System::String^ imagePath;
		std::string img;
		img = "..//Data//Input//N_N_Salt_Pepper.png";

		imagePath = marshal_as<System::String^>(img);
		Image = inputImage(&ImageWidth, &ImageHeight, imagePath);
		
		start_s = clock();

		SubHeight = ImageHeight / size;				// # of Rows for each Processor
		SubSize = SubHeight * ImageWidth;			// # of Pixels for each Processor
		LastSubHeight = ImageHeight % size;			// Row Start Index for the Last Processor
		LastSubSize = LastSubHeight * ImageWidth;	// Pixel Start Index for the Lasst Processor
		
		// Distributing the Image Date Among the Processors
		SendData(Image, size, ImageWidth, ImageHeight);

		// Data after being Sent to the First Processor (Rank 0)
		SubHeight = ImageHeight / size;				// # of Rows for each Processor
		SubSize = SubHeight * ImageWidth;			// # of Pixels for each Processor
		Proc_Data = SubSize;

		// # of Processors Bigger than 1 (Multi Core)
		if (size != 1)
			Proc_Data += ImageWidth;

		New_Init_Pos = 0;				// Returning the Position to Zero
		Sub_Image = new int[Proc_Data]; // ReInitializing the Sub Image Array

		// Coping the Sub Image Data From the Main Image
		for (int i = 0; i < Proc_Data; i++)
			Sub_Image[i] = Image[i];

		// Creating New Array for the Image After Filter
		Sub_Image_Filtered = new int[SubSize + LastSubSize];
		Proc_End_Pos = SubSize;

		// Applying the Filter on the Recieved Data (Master Processors)
		int Sub_Size_Filtered = SubSize + LastSubSize;
		Sub_Image_Filtered = ApplyFilter(Sub_Image, ImageWidth, Sub_Size_Filtered);

		int Pixel = 0;
		// Coping the Filtered Image Partition of the Master & Placing it in the Image Array
		for (int i = 0; i < SubSize; i++)
			Image[Pixel++] = Sub_Image_Filtered[i];
		// Coping the Filtered Image Partition From the other Processors & Placing it in the Image Array
		for (int i = 1; i < size; i++)
		{
			MPI_Recv(&SubSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &Status);
			MPI_Recv(Sub_Image_Filtered, SubSize, MPI_INT, i, 0, MPI_COMM_WORLD, &Status);
			for (int j = 0; j < SubSize; j++)
				Image[Pixel++] = Sub_Image_Filtered[j];
		}

		createImage(Image, ImageWidth, ImageHeight, 0);
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << TotalTime << endl;
		free(Image);
	}
	if (rank != MasterProc)
	{
		MPI_Recv(&SubSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
		MPI_Recv(&ImageHeight, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
		MPI_Recv(&ImageWidth, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
		MPI_Recv(&New_Init_Pos, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
		MPI_Recv(&Proc_Data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
		Sub_Image = new int[Proc_Data];
		MPI_Recv(Sub_Image, Proc_Data, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);

		// Creating New Array for the Image After Filter
		Sub_Image_Filtered = new int[SubSize + LastSubSize];
		Proc_End_Pos = SubSize + ImageWidth;
	
		// Applying the Filter on the Recieved Data (Other Processors)
		int Sub_Size_Filtered = SubSize + LastSubSize;
		Sub_Image_Filtered = ApplyFilter(Sub_Image,ImageWidth, Sub_Size_Filtered);
	
		MPI_Send(&SubSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(Sub_Image_Filtered, SubSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}
