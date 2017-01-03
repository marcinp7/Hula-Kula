SDL_Surface* screen;

class Timer
{
	private:
		int startTicks, pausedTicks;
		bool is_started, is_paused;
	public:
		Timer();
		Timer(int);
		void start(), stop(), pause(), unpause();
		int interval;
		int time();
		bool started(), paused(), finished(), finish(), finishStop();
};
void ApplySurface(int x, int y, SDL_Surface* source)
{
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, NULL, screen, &offset);
}
void ShowText(char* text, int x, int y, TTF_Font* font, SDL_Color textColor)
{
	SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, textColor);
	ApplySurface(x - textSurface->w/2, y - textSurface->h/2, textSurface);
	SDL_FreeSurface(textSurface);
}
void ShowTextF(char* text, int x, int y, TTF_Font* font, SDL_Color textColor)
{
	SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, textColor);
	ApplySurface(x, y, textSurface);
	SDL_FreeSurface(textSurface);
}
Timer::Timer()
{
	is_started = is_paused = false;
	interval = 1000;
}
Timer::Timer(int a)
{
	is_started = is_paused = false;
	interval = a;
}
void Timer::start()
{
	is_started = true;
	is_paused = false;
	startTicks = SDL_GetTicks();
}
void Timer::stop()
{
	is_started = is_paused = false;
}
int Timer::time()
{
	if (is_started)
	{
		if (is_paused) return pausedTicks;
		return SDL_GetTicks() - startTicks;
  }
	return 0;
}
void Timer::pause()
{
	if (is_started && !is_paused)
	{
		is_paused = true;
		pausedTicks = SDL_GetTicks() - startTicks;
	}
}
void Timer::unpause()
{
	if (is_paused)
	{
		is_paused = false;
		startTicks = SDL_GetTicks() - pausedTicks;
	}
}
bool Timer::started()
{
	return is_started;
}
bool Timer::paused()
{
	return is_paused;
}
bool Timer::finished()
{
	if (interval > 0) return time() >= interval;
	else return false;
}
bool Timer::finish()
{
	if (finished())
	{
		start();
		return true;
	}
	else return false;
}
bool Timer::finishStop()
{
	if (finished())
	{
		stop();
		return true;
	}
	else return false;
}

