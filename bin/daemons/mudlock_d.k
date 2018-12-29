#define SAVEFILE "/data/d/mudlock_d/mudlock_d"

int mudlock;

void save_me()
{
    save_object(SAVEFILE);
}

void create()
{
    if(!restore_object(SAVEFILE))
    {
        mudlock = 0;
        save_me();
    }
}

void toggle_mudlock()
{
    mudlock = !mudlock;
    save_me();
}

nomask int query_mudlock()
{
    return mudlock;
}
