#include <thread>
#include <iostream>

class Test
{
public:
	void Laske() {
		int i = 0;
		while(true)
		{
			std::cout << i << std::endl;
			i++;
		}
	};
};

int main()
{
	Test testi;
	std::thread test(&Test::Laske, testi);
	std::cin.get();
}