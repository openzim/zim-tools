
class progress_bar                                  //Class for implementing a progress bar(used in redundancy, url and MIME checks).
{
private:
    char icon;      //Character to be displayed
    int max_icons;  //maximum no. of characters to be displayed.
    int max_no;     //Maximum no of times report() will be called.
    int no;         //number of characters displayed(at a particular time). Similar to the counter variable, except it counts the number of times the character is displayed.
    int counter;    //Number of times report() has been called(at a particular time).
    bool is_initialised;//Boolean value to store wether the object has been initialised or not. report() will not display any characters
    //if the is_initialised value is false.
public:
    progress_bar(char icon_,int max_n)
    {
        if(max_n < 1)
        {
            is_initialised=false;
            return;
        }
        if(max_n < 1)
        {
            is_initialised=false;
            return;
        }
        is_initialised=true;
        max_icons = 80;
        icon=icon_;
        max_no=max_n;
        no = 0;
        counter = 0;
        return;
    }
    void report()
    {
        if(!is_initialised)
            return;
        counter++;
        float i = (counter*1.0)/max_no;
        float disp = (no*1.0)/max_icons;
        while( (disp < i) && (no < max_icons))
        {
            disp = ( no * 1.0) /max_icons;
            no++;
            std::cout<<icon<<std::flush;
        }
        if(no >= max_icons)
            is_initialised=false;
        return;
    }
    void initialise(char icon_,int max_n)
    {
        if(max_n < 1)
        {
            is_initialised=false;
            return;
        }
        is_initialised=true;
        max_icons = 80;
        icon=icon_;
        max_no=max_n;
        no = 0;
        counter = 0;
        return;
    }
    void initialise(char icon_,int max_n,int max_ic)
    {
        if(max_n < 1)
        {
            is_initialised=false;
            return;
        }
        if(max_ic < 1)
        {
            is_initialised=false;
            return;
        }
        max_icons = max_ic;
        is_initialised = true;
        icon = icon_;
        max_no = max_n;
        no = 0;
        counter = 0;
        return;
    }
};
