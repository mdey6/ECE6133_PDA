// MD
// #ifndef ALGORITHMS_CC
// #define ALGORITHMS_CC

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "PE.h"
#include "PE.cc"
#include "Readfile.h"
#include "Readfile.cc"
#include "DrawFloorplan.cc"
#include "Algorithms.h"

using namespace std;

vector<char*> value;
vector<float> width;
vector<float> height;
vector<float> x;
vector<float> y;


DrawFloorplan df;

PE* myPE;

int Argc;
char** Argv;
string Fname;

// Runs the entire polish expression floorplanning
void Algorithms::SA_Call(string fname, int argc, char**& argv, int option)
{
	// clear original PE storage files
    remove("polish.txt");
	ofstream myfile;
	myfile.open ("polish.txt", ios::app);
	if(myfile.is_open())
	{
		//myfile << "Writing this to a file.\n";
	}


    Argc = argc;
    Argv = argv;
    Fname = fname;

    // all variables
    size_t num_modules, sizeofPE, num_nets;
    size_t count = 0;
    int operandCount = 0;
    int operatorCount = 0;
    
    Readfile rf;
    
    // obtain the total number of modules and length of PE 
    num_modules = rf.getNumModules(fname);
    sizeofPE = 2 * num_modules - 1;
    num_nets = rf.getNetSize(fname);

    //PE* myPE;
    myPE = new PE(num_nets, sizeofPE);

    // load PE Attributes from specified file
    string PE_file[num_modules];
    bool Operand_file[num_modules];
    float Width_file[num_modules];
    float Height_file[num_modules];
    bool isSoft_file[num_modules];
    float Area_file[num_modules];
    float LAspect_file[num_modules];
    float HAspect_file[num_modules];

    rf.loadPEAttributes(fname, sizeofPE, PE_file, Operand_file, Width_file, Height_file, isSoft_file, Area_file, LAspect_file, HAspect_file);

	// assign read values to appropriate data structures 
    for (int i = 0; i < sizeofPE; i++) 
    {
    
        if((i == 0) || (i % 2 == 1))
        {
            myPE->NodeAt(i)->StoreValue(PE_file[count].c_str());
            myPE->NodeAt(i)->StoreOperand(Operand_file[count]);

            if(isSoft_file[count] == 0) // hard module
            {
                myPE->NodeAt(i)->StoreHeight(Height_file[count]);
                myPE->NodeAt(i)->StoreWidth(Width_file[count]);
            }
            else // soft module
            {
                myPE->NodeAt(i)->StoreHeight(sqrt(Area_file[count] / ((LAspect_file[count] + HAspect_file[count])/2.0)));
                myPE->NodeAt(i)->StoreWidth(sqrt(Area_file[count] * ((LAspect_file[count] + HAspect_file[count])/2.0)));
            }
            count++;
        }
        else
        {
            srand(time(NULL) * i);
            if(rand()%2 == 0)
                myPE->NodeAt(i)->StoreValue("*"); 
            else
                myPE->NodeAt(i)->StoreValue("+"); 

            myPE->NodeAt(i)->StoreOperand(0);
            myPE->NodeAt(i)->StoreHeight(0);
            myPE->NodeAt(i)->StoreWidth(0);  
        }
        
        if(myPE->NodeAt(i)->Operand())
            myPE->NodeAt(i)->StoreId(operandCount++);
        else
            myPE->NodeAt(i)->StoreId(operatorCount++);
                    
    }
    
    // initialize dimensions and create tree
    myPE->CreateFastAccessModule();
    myPE->CreateTree();
    myPE->InitializeDimensions();

    // Print PE
    myPE->PrintPostOrder();

    // Store initial floorplan info
    myfile<<endl<<"Initial"<<endl;

    for(int i = 0; i < sizeofPE; i++)
    {
        if(myPE->NodeAt(i)->Operand())  // if it is an operand
        {
            myfile<<myPE->NodeAt(i)->Value()<<" "<<myPE->NodeAt(i)->X()<<" "<<myPE->NodeAt(i)->Y()<<" "<<myPE->NodeAt(i)->Width()<<" "<<myPE->NodeAt(i)->Height()<<" "<<myPE->NodeAt(i)->CentroidX()<<" "<<myPE->NodeAt(i)->CentroidY()<<endl;
        }
        else
        {
        
        }   
    }
    
    // Load Net Attributes from file  
    vector<string> AllNets_file;
    vector<vector<string>> AllNetlists_file;
    AllNetlists_file = rf.loadNetAttributes(fname, AllNets_file);

    count = 0;

	// Store net details in appropriate data structures
    for (size_t i = 0; i < AllNets_file.size(); i++) 
    {
        if(AllNetlists_file[i].size() > 1)
        {
            myPE->NetAt(count)->StoreValue(AllNets_file[i].c_str());
            
            for(int j = 0; j < AllNetlists_file[i].size(); j++) 
            {
                myPE->UpdateNetlist(count, (AllNetlists_file[i][j]).c_str());
            }
            count++;
         }
    }
    
    // Print Netlist  
    myPE->PrintNetlist();
    myPE->PrintNetsPerModule();
    
    // Compute HPWL
    myPE->ComputeHpwl();
    myPE->InitializeTotalHpwl();

    myPE->PrintAllHpwl();


    
    time_t seconds1, seconds2, seconds3, seconds4;
    
    float initialHpwl = myPE->TotalHpwl();
    float initialArea = myPE->TotalArea();
    float initialTotalAreaOccupied = myPE->TotalAreaOccupied();
    
    float SAHpwl = 0, SAArea = 0;
    if (option == 0 || option == 2)
    {

		// Simulated Annealing
		cout<<"SIMULATED ANNEALING"<<endl;
		//cout << "\nhahahaha\n";
		seconds1 = time(NULL);
		myPE->Sa();    
		seconds2 = time(NULL);
		
		myfile<<endl<<"Annealing"<<endl;

		for(int i = 0; i < sizeofPE; i++)
		{
		    if(myPE->NodeAt(i)->Operand())  // if it is an operand
		    {
		        myfile<<myPE->NodeAt(i)->Value()<<" "<<myPE->NodeAt(i)->X()<<" "<<myPE->NodeAt(i)->Y()<<" "<<myPE->NodeAt(i)->Width()<<" "<<myPE->NodeAt(i)->Height()<<" "<<myPE->NodeAt(i)->CentroidX()<<" "<<myPE->NodeAt(i)->CentroidY()<<endl;
		    }
		    else
		    {
		    
		    }   
		}
		myPE->PrintPostOrder();
		

		SAHpwl = myPE->TotalHpwl();
		SAArea = myPE->TotalArea();
    
    }

    // Stockmeyer algorithm

    float finalHpwl = 0, finalArea = 0;
    if(option == 1 || option == 2)
    {

		seconds3 = time(NULL);
		myPE->Stockmeyer();
		seconds4 = time(NULL);

		myfile<<endl<<"Stockmeyer"<<endl;

		for(int i = 0; i < sizeofPE; i++)
		{
		    if(myPE->NodeAt(i)->Operand())  // if it is an operand
		    {
		        myfile<<myPE->NodeAt(i)->Value()<<" "<<myPE->NodeAt(i)->X()<<" "<<myPE->NodeAt(i)->Y()<<" "<<myPE->NodeAt(i)->Width()<<" "<<myPE->NodeAt(i)->Height()<<" "<<myPE->NodeAt(i)->CentroidX()<<" "<<myPE->NodeAt(i)->CentroidY()<<endl;
		    }
		    else
		    {
		    
		    }   
    	}


		myPE->PrintPostOrder();
		myPE->PrintNetlist();
		myPE->PrintNetsPerModule();
		
		myPE->PrintAllHpwl();
		
		finalHpwl = myPE->TotalHpwl();
		finalArea = myPE->TotalArea();
    }

	// display stats
    
    cout<<endl<<endl<<"\t\t\tSTATISTICS:"<<endl<<endl;
    cout<<"Num modules: "<<num_modules<<endl;
    cout<<"Size of PE is "<<sizeofPE<<endl;
    cout<<"Number of nets: "<<num_nets<<endl<<endl;
    cout<<"\tRUN TIME"<<endl<<endl;
    cout<<"SA Run Time: "<<seconds2-seconds1<<" secs"<<endl;
    cout<<"Stockmeyer Run Time: "<<seconds4-seconds3<<"secs"<<endl; 
    cout<<endl<<"\tHPWL"<<endl<<endl;
    cout<<"Intial HPWL = "<<initialHpwl<<endl;
    cout<<"SA HPWL = "<<SAHpwl<<endl;
    cout<<"Stockmeyer HPWL = "<<finalHpwl<<endl;
    cout<<"SA HPWL reduction = "<<(initialHpwl - SAHpwl)/initialHpwl*100<<" %"<<endl;
    cout<<"Stockmeyer HPWL reduction = "<<(initialHpwl - finalHpwl)/initialHpwl*100<<" %"<<endl;
    cout<<endl<<"\tTOTAL CHIP AREA"<<endl<<endl;
    cout<<"Intial Chip Area = "<<initialArea<<endl;
    cout<<"SA Chip Area = "<<SAArea<<endl;
    cout<<"Stockmeyer Chip Area = "<<finalArea<<endl;
    cout<<"SA Chip Area reduction = "<<(initialArea - SAArea)/initialArea*100<<" %"<<endl;
    cout<<"Stockmeyer Chip Area reduction = "<<(initialArea - finalArea)/initialArea*100<<" %"<<endl;
    cout<<endl<<"\tAREA UTILIZATION"<<endl<<endl;
    cout<<"Total Area Occupied by modules = "<<initialTotalAreaOccupied<<endl;
    cout<<"Total whitespace after SA = "<<(SAArea - initialTotalAreaOccupied)/SAArea*100<<" %"<<endl;
    cout<<"Area utilization after SA = "<<(initialTotalAreaOccupied/SAArea*100)<<" %"<<endl;
    cout<<"Total whitespace after Stockmeyer = "<<(finalArea - initialTotalAreaOccupied)/finalArea*100<<" %"<<endl;
    cout<<"Area utilization after Stockmeyer = "<<(initialTotalAreaOccupied/finalArea*100)<<" %"<<endl;

    // call GUI
    df.drawFloorplan(fname, num_modules, argc, argv);

    delete myPE;
	myfile.close();

     
}



// #endif
// DM
