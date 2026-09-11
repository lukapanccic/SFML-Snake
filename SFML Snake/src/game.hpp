#include <SFML/Graphics.hpp>
#include "snake.hpp"
#include "food.hpp"
#include <random>

class Game {
	int score;
	bool gameOver;
	Snake snake;
	Food food;
	std::mt19937 random;
public:
	Game(std::mt19937 random);
	void increaseScore(int amount);
	void setGameOver(bool state);
	void draw(sf::RenderWindow& window, const sf::Font& font);
	bool isGameOver() const;
	int getScore() const;
	void handleInput(sf::Keyboard::Key key);
	Snake& getSnake();
	Food& getFood();
	void update();
};