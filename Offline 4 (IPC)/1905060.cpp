#include <iostream>
#include <fstream>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <random>
#include <chrono>
#include <algorithm>
#define PRINTING_STATION 4
#define BINDING_STATION 2
#define MAX_READER 2
#define AVAILABLE -1

using namespace std;

int n, m, w, x, y;
int *binder;
int no_of_groups;

fstream output;
string output_file_name = "output.txt";

pthread_t *student_thread, *groupleader_thread;
pthread_t library_thread;
pthread_cond_t *binding_station_condition;
//time_t start_time, curtime;
auto start_time = chrono::high_resolution_clock::now();

vector<int> printing_station_student_queue[PRINTING_STATION];

pthread_mutex_t printing_station_mutex[PRINTING_STATION], printing_station_wait[PRINTING_STATION], binding_station_mutex[BINDING_STATION];

int printing_station_queue[PRINTING_STATION];

pthread_mutex_t printing_mutex, creating_mutex, binding_station_wait_mutex, binding_prep_mutex, binding_finish_mutex, book_entry_mutex;

sem_t readers_semaphore;

pthread_cond_t printing_station_condition[PRINTING_STATION];

int printing_signal[PRINTING_STATION] = {-1, -1, -1, -1};


int binding_station_state[BINDING_STATION] = {-1, -1};
int number_of_submissions = 0, readers_count = 0;

pthread_cond_t binding_done_condition, reader_condition;



bool isLeader(int student_id)
{
    return (student_id % m == 0);
}

int student_group(int student_id)
{
    double result = ceil((double)student_id / (double)m);
    return (int)result;
}

void mutex_init(pthread_mutex_t mutex[], int size)
{
    for (int i = 0; i < size; i++)
    {
        mutex[i] = PTHREAD_MUTEX_INITIALIZER;
    }
}

void mutex_init(pthread_mutex_t mutex)
{
    mutex = PTHREAD_MUTEX_INITIALIZER;
}

void condition_init(pthread_cond_t cond[], int size)
{
    for (int i = 0; i < size; i++)
    {
        cond[i] = PTHREAD_COND_INITIALIZER;
    }
}

void condition_init(pthread_cond_t cond)
{
    cond = PTHREAD_COND_INITIALIZER;
}

void sem_init(sem_t sem, int value)
{
    sem_init(&sem, 0, value);
}

void sem_wait(sem_t sem)
{
    sem_wait(&sem);
}

void sem_post(sem_t sem)
{
    sem_post(&sem);
}

void sem_destroy(sem_t sem)
{
    sem_destroy(&sem);
}

bool searchqueue(int id)
{
    int group_num = student_group(id);
    int x;
    vector<int> temp = printing_station_student_queue[id % PRINTING_STATION];
    for (int n = 0; n < temp.size(); n++)
    {
        x = student_group(temp.at(n));
        if (x == group_num)
        {
            return true;
        }
    }
    return false;
}

void mutex_lock(pthread_mutex_t mutex)
{
    pthread_mutex_lock(&mutex);
}

void mutex_unlock(pthread_mutex_t mutex)
{
    pthread_mutex_unlock(&mutex);
}

void condition_wait(pthread_cond_t cond, pthread_mutex_t mutex)
{
    pthread_cond_wait(&cond, &mutex);
}

void condition_signal(pthread_cond_t cond)
{
    pthread_cond_signal(&cond);
}

void condition_broadcast(pthread_cond_t cond)
{
    pthread_cond_broadcast(&cond);
}

void mutex_destroy(pthread_mutex_t mutex)
{
    pthread_mutex_destroy(&mutex);
}

void condition_destroy(pthread_cond_t cond)
{
    pthread_cond_destroy(&cond);
}

bool removefromqueue(int q, int id)
{
    int index = 0;
    while (printing_station_student_queue[q].size() >= index)
    {
        if (printing_station_student_queue[q].at(index) == id)
        {
            printing_station_student_queue[q].erase(printing_station_student_queue[q].begin() + index);
            return true;
        }

        index++;
    }
    return false;
}

struct student
{
    int id;
};

struct student* init_student(struct student *s, int id)
{
    s->id = id;
    return s;
}

int generateRandomNumber(int min_val, int max_val)
{
    random_device rd;
    mt19937 gen(rd());
    poisson_distribution<int> dis((min_val - max_val) / 2);
    return dis(gen) + min_val;
}

int calculate_time()
{
    auto curtime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(curtime - start_time);
    return duration.count();
}


void *printing_task(void *ptr)
{
    student *s = (student *)ptr;
    pthread_mutex_lock(&printing_mutex);
    output.open(output_file_name, ios::app);
    output << "Student " << s->id << " has arrived at the print station at time " << calculate_time() << endl;
    output.close();
    pthread_mutex_unlock(&printing_mutex);

    int desired_ps = (s->id % PRINTING_STATION);

    pthread_mutex_lock(&printing_station_wait[desired_ps]);

    int chk = 0;
    while (true)
    {
        if (chk == 0)
        {
            printing_station_student_queue[desired_ps].push_back(s->id);
            chk++;
        }
        else
        {
            if (printing_signal[desired_ps] == -1)
            {
                if (printing_signal[desired_ps] == student_group(s->id))
                {
                    removefromqueue(desired_ps, s->id);
                    break;
                }
                else if (printing_station_queue[desired_ps] == 1)
                {
                    pthread_cond_wait(&printing_station_condition[desired_ps], &printing_station_wait[desired_ps]);
                    if (printing_signal[desired_ps] == -1)
                    {
                        removefromqueue(desired_ps, s->id);
                        break;
                    }
                }
                else
                {
                    removefromqueue(desired_ps, s->id);
                    break;
                }
            }
            else
            {
                pthread_cond_wait(&printing_station_condition[desired_ps], &printing_station_wait[desired_ps]);
            }
        }
    }

    pthread_mutex_unlock(&printing_station_wait[desired_ps]);

    pthread_mutex_lock(&printing_station_mutex[desired_ps]);

    printing_station_queue[desired_ps] = 1;
    int delay = generateRandomNumber(10, 17);
    printing_signal[desired_ps] = -2;
    sleep(delay);

    printing_station_queue[desired_ps] = -1;
    pthread_mutex_unlock(&printing_station_mutex[desired_ps]);
    bool flag = false;

    pthread_mutex_lock(&printing_mutex);

    if (searchqueue(s->id))
    {
        int o = student_group(s->id);
        printing_signal[desired_ps] = o;
        pthread_cond_signal(&printing_station_condition[desired_ps]);
    }
    else
    {
        printing_signal[desired_ps] = -1;
        pthread_cond_signal(&printing_station_condition[desired_ps]);
    }
    sleep(5);

    pthread_mutex_unlock(&printing_mutex);

    pthread_mutex_lock(&printing_mutex);

    printing_signal[desired_ps] = -1;
    pthread_cond_signal(&printing_station_condition[desired_ps]);

    pthread_mutex_unlock(&printing_mutex);

    pthread_mutex_lock(&binding_station_wait_mutex);

    binder[student_group(s->id) - 1]++;
    if (binder[student_group(s->id) - 1] == m)
    {   
        output.open(output_file_name, ios::app);
        output << "Group " << student_group(s->id) << " has finished printing at time " << calculate_time() << endl;
        output.close();
        pthread_cond_signal(&binding_station_condition[student_group(s->id) - 1]);
    }

    pthread_mutex_unlock(&binding_station_wait_mutex);

    return NULL;
}

void *submission_task(void *ptr)
{
    // this is the library thread
    int person = *(int *)ptr;

    while (true)
    {
        int delay = generateRandomNumber(4, 10);
        sleep(delay);
        
        sem_wait(&readers_semaphore);

        pthread_mutex_lock(&book_entry_mutex);
        readers_count = readers_count + 1;
        output.open(output_file_name, ios::app);
        output << "Staff " << person << " has started reading the entry book at time " << calculate_time() << ". No of submissions = " << number_of_submissions << endl;
        output.close();
        pthread_mutex_unlock(&book_entry_mutex);
        sleep(generateRandomNumber(3, 6));

        pthread_mutex_lock(&book_entry_mutex);
        readers_count = readers_count -1;
        if (number_of_submissions == no_of_groups)
        {
            pthread_mutex_unlock(&book_entry_mutex);
            break;
        }
        if (readers_count == 0)
        {
            pthread_cond_signal(&reader_condition);
        }
        pthread_mutex_unlock(&book_entry_mutex);
        // up the readers semaphore
        sem_post(&readers_semaphore);
    }
    return NULL;
}

void *binding_task(void *ptr)
{
    student *s = (student *)ptr;
    pthread_mutex_lock(&binding_station_wait_mutex);
    while (1)
    {   
        int val = binder[student_group(s->id) - 1];
        if (val != m)
        {
            pthread_cond_wait(&binding_station_condition[student_group(s->id) - 1], &binding_station_wait_mutex);
        }
        else
        {
            break;
        }
    }

    pthread_mutex_unlock(&binding_station_wait_mutex);
    
    int which_binding_station = -1;
    pthread_mutex_lock(&binding_prep_mutex);
    while (1)
    {
        if (binding_station_state[0] == -1)
        {
            // means 1 is free
            which_binding_station = 0;
            break;
        }
        else if (binding_station_state[1] == -1)
        {
            // means 2 is free
            which_binding_station = 1;
            break;
        }
        else
        {
            // means both are busy
            pthread_cond_wait(&binding_done_condition, &binding_prep_mutex);
        }
    }
    pthread_mutex_unlock(&binding_prep_mutex);
    // now binding
    pthread_mutex_lock(&binding_station_mutex[which_binding_station]);
    binding_station_state[which_binding_station] = 1;

    int delay = generateRandomNumber(5, 10);
    sleep(delay);
    
    output.open(output_file_name, ios::app);
    output << "Group " << student_group(s->id) << " has finished binding at time " << calculate_time() << endl;
    output.close();

    pthread_mutex_unlock(&binding_station_mutex[which_binding_station]);

    pthread_mutex_lock(&binding_finish_mutex); // lock to protoect cond variable
    
    binding_station_state[which_binding_station] = -1;

    pthread_cond_signal(&binding_done_condition);
    
    pthread_mutex_unlock(&binding_finish_mutex);

    // now all of them are done binding and prepared for the submission
    pthread_mutex_lock(&book_entry_mutex);
    
    while (readers_count > 0)
    {
        
        pthread_cond_wait(&reader_condition, &book_entry_mutex);
    }

    // now all readers are done
    delay = generateRandomNumber(1, 8);
    sleep(delay);
    number_of_submissions = number_of_submissions + 1;
    output.open(output_file_name, ios::app);
    output << "Group " << student_group(s->id) << " has submitted the report at time " << calculate_time() << endl;
    output.close();
    pthread_mutex_unlock(&book_entry_mutex);

    return NULL;
}

int main()
{
    fstream file;
    file.open("input.txt", ios::in);
    file >> n >> m >> w >> x >> y;
    file.close();
    no_of_groups = n / m;
    binder = new int[n / m];
    for(int i=0;i<n/m;i++){
        binder[i] = 0;
    }
    student_thread = new pthread_t[n];
    groupleader_thread = new pthread_t[no_of_groups];
    for (int i = 0; i < 4; i++){
        printing_station_queue[i] = -1;
    }
    sem_init(&readers_semaphore, 0, 2);
    
    mutex_init(printing_station_mutex, 4);
    mutex_init(printing_station_wait, 4);
    mutex_init(binding_station_mutex, 2);
    mutex_init(printing_mutex);
    mutex_init(creating_mutex);
    mutex_init(binding_station_wait_mutex);
    mutex_init(binding_prep_mutex);
    mutex_init(binding_finish_mutex);
    mutex_init(book_entry_mutex);
    
    //vector<student *> allstudents;
    
    binding_station_condition = new pthread_cond_t[no_of_groups];
    for (int i = 0; i < no_of_groups; i++){
        pthread_cond_init(&binding_station_condition[i], NULL);
    }
    for (int i = 0; i < PRINTING_STATION; i++){
        pthread_cond_init(&printing_station_condition[i], NULL);
    }

    int group_no = 0;
    int a = 1, b = 2;

    pthread_cond_init(&binding_done_condition, NULL);
    pthread_cond_init(&reader_condition, NULL);

    struct student s[n];

    // vector<int> arr;
    // for (int i = 0; i < n; i++){
    //     arr.push_back(i);
    // }
    // random_shuffle(arr.begin(), arr.end());

    int i;
    for (int i = 0; i < n; i++)
    {
        //i = arr.at(j);
        pthread_mutex_lock(&creating_mutex);
        struct student *temp = init_student(&s[i], i + 1);

        int delay = generateRandomNumber(1, 7);
        sleep(delay);
        pthread_create(&student_thread[i], NULL, printing_task, (void *)temp);

        if(isLeader(i+1)){
            group_no++;
            pthread_create(&groupleader_thread[group_no], NULL, binding_task, (void *)temp); // the thread will call the simulateSubmitReport function;
        } 
        pthread_mutex_unlock(&creating_mutex);
    }

    pthread_create(&library_thread, NULL, submission_task, (void *)&a);
    pthread_create(&library_thread, NULL, submission_task, (void *)&b);
    
    for (int i = 0; i < no_of_groups; i++)
    {
        pthread_join(groupleader_thread[i], NULL); 
    }
    for (int i = 0; i < n; i++)
    {
        pthread_join(student_thread[i], NULL); 
    }
    for (int i = 0; i < MAX_READER; i++)
    {
        pthread_join(library_thread, NULL); 
    }
    return 0;
}