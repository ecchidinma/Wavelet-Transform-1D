/* 
 * File:   main.cpp
 * Author: Emmanuel C. Chidinma
 * emmanuel.c.chidinma@gmail.com
 * 
 * 1D (Audio) Haar discrete wavelet transform (DWT) and then the inverse DWT
 *
 * This is optimized for DSP Processors and has been ported to an embedded
 * DSP platform; thus, in order to manage memory, NO scratch array is used.
 * The transform is done in-place. Although this is a C++ program, the use 
 * of classes, the bool variable type etc have intentionally been omitted
 * for easier porting to embedded (ANSI) c. For the same reason, recursive 
 * algorithms have been avoided.
 *
 * length of the array(s) must be dyadic: a power of 2 eg, 2, 4, 8....1024 etc
 *
 * Created on December 2, 2016, 8:35 PM
 */
#include <iostream>
//#include <cmath>
//#include <cstdlib>

//using namespace std;

const unsigned int NUM_ELEMENTS = 8;
const float SQRT_2 = 1.414214f;
//const double SQRT_2 = 1.414213562373095;

unsigned char validateLength(unsigned short* pI, unsigned int length);
unsigned short inputAndValidation(unsigned short* pI);
void waveletTransform(float* arr, unsigned int length, unsigned short level);
void rearrange(float* arr, unsigned int length);
void revertRearrange(float* arr, int length);
void invWaveletTransform(float* arr, unsigned int length, unsigned short level);
unsigned int twoExpLevel(unsigned short iLevel);
void printArr(float* arr, unsigned int length);

int main()
{
    // initialize a test array
    float testArr[NUM_ELEMENTS] = {4.0f, 6.0f, 10.0f, 12.0f, 8.0f, 6.0f, 5.0f, 5.0f};
    
    unsigned short iMaxLevel;
    unsigned short* pI01 = &iMaxLevel;
    
    if(validateLength(pI01, NUM_ELEMENTS) <= 0) return 1;
    std::cout << "Maximum level " << " = " << iMaxLevel << std::endl << std::endl;
	
    // Ask the user to choose the DWT level
    iMaxLevel = inputAndValidation(pI01);
   
    // Perform Haar DWT
    waveletTransform(testArr, NUM_ELEMENTS, iMaxLevel);
    printArr(testArr, NUM_ELEMENTS);
    
    // Perform Haar IDWT
    invWaveletTransform(testArr, NUM_ELEMENTS, iMaxLevel);
    printArr(testArr, NUM_ELEMENTS);
   
    std::cin.get(); // press enter to close screen
    return 0;
}// end main()

unsigned char validateLength(unsigned short* pI, unsigned int length) // unsigned char used instead of bool to maintan compatibility with ANSI C
{
    char ch = (((length == 0) || (length == 1)) ? 'a' : (((length % 2) == 1) ? 'b' : 'c'));
    
    switch(ch)
    {
        case 'a':   
            std::cout << "Length of array cannot be 0 or 1" << std::endl;
            return 0;
        case 'b':
            std::cout << "Length of array cannot be odd" << std::endl;
            return 0;
        case 'c': // here, derive the log of length in base 2
            *pI = 1; // initialize contents of pointer to 1 since length is already even
            length /= 2;
            do
            {
                if((length % 2) == 1)
                {
                    std::cout << "Length of array is not a power of 2" << std::endl;
                    return 0;
                }
                (*pI)++;
                length /= 2;
            }while(length != 1); // end do-while
            return 1;
    }// end switch
}// end validateLength()

unsigned short inputAndValidation(unsigned short* pI)
{
   // assign an arbitrary, out-of-range value to ensure 
   // iLevel to ensure validation 
   unsigned short iLevel = *pI + 10;

   //validate the user's input
   while (!((0 <= iLevel)&&(iLevel <= *pI)))
   {
     iLevel = *pI; // default level value in case user did not enter any level number
     std::cout << "Please Choose a +ve DWT Level Less Than Or Equal To " << *pI << std::endl;
     std::cin >> iLevel;
     while (std::cin.get() != '\n');  // flush out any remaining newline xcters in input buffer
   }// end while
   return iLevel;
}//end inputAndValidation()

void waveletTransform(float* arr, unsigned int length, unsigned short level)
{
    std::cout << "This is level " << level << " DWT Computation." << std::endl << std::endl;
    
    while(level--)
    {
        float fTemp01, fTemp02;
        for(int i = 0; i < length; i+=2) // only even indices
        {
            fTemp01 = *(arr + i); fTemp02 = *(arr + i + 1);
            *(arr + i) = (fTemp01 + fTemp02)/SQRT_2; // trend
            *(arr + i + 1) = (fTemp01 - fTemp02)/SQRT_2; // fluctuation
        }// end for
        rearrange(arr, length);
        length /= 2;
    }// end while
}// end waveletTransform()

void rearrange(float* arr, unsigned int length)
{
    //FOR AN ARRAY OF EVEN LENGTH length (16), THE NUMBER OF ODD INDICES IS length/2 (8)
    // THEREFORE THE NUMBER OF ODD INDICES WITHIN THE LOWER-HALF-RANGE IS 4 (=length/4 ie 1, 3, 5, 7)
    // TO OBTAIN THEIR INDICES WITHIN A NEW ARRAY OF length/4 ELEMENTS, DO THE
    // FOLLOWING INTEGER MATH: 1/2, 3/2, 5/2, AND 7/2 WHICH YIELDS: 0, 1, 2, 3 
    int quartLen = length/4;
    unsigned char indexMask[quartLen]; // unsigned char used instead of bool to maintan compatibility with ANSI C
    // initialize to zero
    for(int i = 0; i < quartLen; i++)
    {
        *(indexMask + i) = 0;
    }// end for
   
    int lastMidOddIndex = (length/2) - 1;
    //consider only odd indices up to the middle
    for(int i = 1; i <= lastMidOddIndex; i+=2)
    {
        //IF THE CORRESPONDING FLAG TO THIS ODD INDEX IS NON-ZERO IT MEANS THAT INDEX HAS
        // ALREADY BEEN CONSIDERED. IN THAT CASE PLEASE MOVE ON TO THE NEXT ODD INDEX
        // OF COURSE ARRAY indexMask MUST NOT HAVE ZERO ALLOCATION (quartLen > 0)
        if((quartLen > 0) && *(indexMask + (i/2))) continue; // // remember the integer math with odd number explained in the 1st few lines at the start of rearrange()
        
        int indexTemp01;
        float valueTemp;
        int indexTemp02 = i;
        
        //DEDUCE NEW INDEX LOCATIONS AND COPY ARRAY VALUES UNTIL WE RETURN TO THE STARTING INDEX
        do
        {
            if((indexTemp02 % 2) == 0) // true for even index
            {//if_else_a_begins.
                //calculate the new index for an old even index
                indexTemp01 = indexTemp02/2;
                
                if(indexTemp01 == i) //true if calculated index has matched initial index
                {// end if_else_a_begins
                    *(arr + i) = valueTemp;
                    break; //leave do-while loop
                }// end if_else_b_ctd.
                else
                {
                    // ASSIGN NEW POSITION
                    *(arr + i) = *(arr + indexTemp01); //save the value whose new index location we shall find next to arr[i], now a scratch array space, as the temp location
                    *(arr + indexTemp01) = valueTemp; // assign the last saved value whose index was used to deduce indexTemp01
                    valueTemp = *(arr + i); // save the value whose new index location we shall find next to the temp variable
                    indexTemp02 = indexTemp01; // assign index whose new index we shall find next
                }// if_else_b_ends
            }// if_else_a_ctd.
            else //((indexTemp02 % 2) != 0) // true for odd index
            {//NB: the 1st iteration of indexTemp02 (=i) must come here since i is always an odd index
                //calculate the new index for an old odd index
                indexTemp01 = indexTemp02 + lastMidOddIndex - (indexTemp02 - 1)/2;
                
                if(indexTemp02 == i) //this condition can only be true only once in this else clause of if_else_a block
                {// if_else_c_begins
                    // ASSIGN NEW POSITION
                    valueTemp = *(arr + indexTemp01); // save the value whose new index location we shall find next to the temp variable
                    // assign value at old index to that at the new index
                    *(arr + indexTemp01) = *(arr + indexTemp02); // the implication of this is that arr[i] can now be used as a scratch space
                }// if_else_c_ctd.
                else // for subsequent values of indexTemp02 before next i from the outer for loop
                {
                    // ASSIGN NEW POSITION
                    *(arr + i) = *(arr + indexTemp01); //save the value whose new index location we shall find next to arr[i], now a scratch array space, as the temp location
                    *(arr + indexTemp01) = valueTemp; // assign the last saved value whose index was used to deduce indexTemp01
                    valueTemp = *(arr + i); // save the value whose new index location we shall find next to the temp variable

                    //CHECK WHETHER ANY POTENTIAL FUTURE ODD INDEX (THAT IS IN THE LOWER-HALF-RANGE) HAS BEEN DEDUCED FOR CONSIDERATION
                    // AND SET ITS FLAG SO THAT IT WILL NOT BE CONSIDERED AGAIN BY THE OUTER FOR LOOP
                    if((indexTemp02 <= lastMidOddIndex) && (quartLen > 0)) // odd index within mid range provided array indexMask doesn't have a zero allocation
                    {
                        *(indexMask + (indexTemp02/2)) = 1; // remember the integer math with odd number explained in the 1st few lines at the start of rearrange()
                    }//end if
                }// if_else_c_ends
                indexTemp02 = indexTemp01; // assign  index whose new index we shall find next
            }// if_else_a_ends
        }while(indexTemp02 != i); // end do-while
    }// end for
}// end rearrange()

void revertRearrange(float* arr, int length)
{
    //FOR AN ARRAY OF EVEN LENGTH length (16), THE NUMBER OF EVEN INDICES IS length/2 (ie 8,call it halfL)
    // THEREFORE THE NUMBER OF EVEN INDICES WITHIN THE UPPER-HALF-RANGE IS 4 (=length/4 ie 8, 10, 12, 14)
    // TO OBTAIN THEIR INDICES WITHIN A NEW ARRAY OF length/4 ELEMENTS, DO THE FOLLOWING INTEGER
    // MATH: (8-halfL)/2, (10-halfL)/2, (12-halfL)/2, AND (14-halfL)/2 WHICH YIELDS: 0, 1, 2, 3 
    int quartLen = length/4;
    unsigned char indexMask[quartLen];
    // initialize to zero
    for(int i = 0; i < quartLen; i++)
    {
        indexMask[i] = 0;
    }// end for
    
    int lastEvenIndex = length - 2;
    int halfLen = length/2; // this is also the first even index in the upper-half-range
    //consider only even indices from the middle up to the end
    for(int i = halfLen; i <= lastEvenIndex; i+=2)
    {
        //IF THE CORRESPONDING FLAG TO THIS ODD INDEX IS NON-ZERO IT MEANS THAT INDEX HAS
        // ALREADY BEEN CONSIDERED. IN THAT CASE PLEASE MOVE ON TO THE NEXT ODD INDEX
        // OF COURSE ARRAY indexMask MUST NOT HAVE ZERO ALLOCATION (quartLen > 0)
        if((quartLen > 0) && indexMask[(i - halfLen)/2]) continue; 
        
        int indexTemp01;
        float valueTemp;
        int indexTemp02 = i;
        
        //DEDUCE NEW INDEX LOCATIONS AND COPY ARRAY VALUES UNTIL WE RETURN TO THE STARTING INDEX
        do
        {
            if(indexTemp02 < halfLen) // true for lower-half-range
            {//if_else_a_begins.
                //calculate the new index for an old lower-half-range index
                indexTemp01 = 2*indexTemp02;
                
                if(indexTemp01 == i) //true if calculated index has matched initial index
                {// end if_else_a_begins
                    arr[i] = valueTemp;
                    break; //leave do-while loop
                }// end if_else_b_ctd.
                else
                {
                    // ASSIGN NEW POSITION
                    arr[i] = arr[indexTemp01]; //save the value whose new index location we shall find next to arr[i], now a scratch array space, as the temp location
                    arr[indexTemp01] = valueTemp; // assign the last saved value whose index was used to deduce indexTemp01
                    valueTemp = arr[i]; // save the value whose new index location we shall find next to the temp variable
                    indexTemp02 = indexTemp01; // assign index whose new index we shall find next
                }// if_else_b_ends
            }// if_else_a_ctd.
            else //(indexTemp02 >= halfLen) // true for upper-half-range
            {//NB: the 1st iteration of indexTemp02 (=i) must come here since i is always an upper-half-range index
                //calculate the new index for an old upper-half-range index
                indexTemp01 = 2*indexTemp02 - lastEvenIndex - 1;
                
                if(indexTemp02 == i) //this condition can only be true only once in this else clause of if_else_a block
                {// if_else_c_begins
                    // ASSIGN NEW POSITION
                    valueTemp = arr[indexTemp01]; // save the value whose new index location we shall find next to the temp variable
                    // assign value at old index to that at the new index
                    arr[indexTemp01] = arr[indexTemp02]; // the implication of this is that arr[i] can now be used as a scratch space
                }// if_else_c_ctd.
                else // for subsequent values of indexTemp02 before next i from the outer for loop
                {
                    // ASSIGN NEW POSITION
                    arr[i] = arr[indexTemp01]; //save the value whose new index location we shall find next to arr[i], now a scratch array space, as the temp location
                    arr[indexTemp01] = valueTemp; // assign the last saved value whose index was used to deduce indexTemp01
                    valueTemp = arr[i]; // save the value whose new index location we shall find next to the temp variable

                    //CHECK WHETHER ANY POTENTIAL FUTURE UPPER-HALF-RANGE INDEX (THAT IS EVEN) HAS BEEN DEDUCED FOR CONSIDERATION
                    // AND SET ITS FLAG SO THAT IT WILL NOT BE CONSIDERED AGAIN BY THE OUTER FOR LOOP
                    if(((indexTemp02 % 2) == 0) && (quartLen > 0)) // upper-half-range index that is even provided array indexMask doesn't have a zero allocation
                    {
                        indexMask[(indexTemp02 - halfLen)/2] = 1; // remember the integer math with odd number explained in the 1st few lines at the start of rearrange()
                    }//end if
                }// if_else_c_ends
                indexTemp02 = indexTemp01; // assign  index whose new index we shall find next
            }// if_else_a_ends
        }while(indexTemp02 != i); // end do-while
    }// end for
}// end revertRearrange()

void invWaveletTransform(float* arr, unsigned int length, unsigned short level)
{
    std::cout << "This is level " << level << " IDWT Computation." << std::endl << std::endl;
    unsigned int runningLength = length/(1 << level); // initial running length deduced
    //OR: unsigned short runningLength = length/twoExpLevel(level); // initial running length deduced
    //std::cout << "This is (1 << level) " << (1 << level) << std::endl << std::endl;
    //std::cout << "This is running length " << runningLength << std::endl << std::endl;
    runningLength *=2;
    while(runningLength <= length)
    {
        //std::cout << "This is running length " << runningLength << std::endl << std::endl;
        revertRearrange(arr, runningLength);
        float fTemp01, fTemp02;
        for(int i = 0; i < runningLength; i+=2) // only even indices
        {
            fTemp01 = *(arr + i); fTemp02 = *(arr + i + 1);
            *(arr + i) = (fTemp01 + fTemp02)/SQRT_2; // sample value
            *(arr + i + 1) = (fTemp01 - fTemp02)/SQRT_2; // the next sample value
        }// end for
        runningLength *=2;
    }// end while
}//invWaveletTransform()

unsigned int twoExpLevel(unsigned short iLevel)
{
    unsigned int expValue = 1;
    for(int i=0; i<iLevel; i++)
    {
        expValue *= 2;
    }
    return expValue;
}//twoExpLevel()

void printArr(float* arr, unsigned int length)
{
    for(int i = 0; i < length; i++)
    {
        std::cout << "value at index " << i << " = " << arr[i] << std::endl;
    }
    std::cout << std::endl;
}// end printArr()

