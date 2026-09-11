#include "game.hpp"
#include <string>

void Game::increaseScore(int amount) 
{
	score += amount;
}

void Game::setGameOver(bool state) 
{
	gameOver = state;
}

void Game::draw(sf::RenderWindow& window, const sf::Font& font)
{
	food.draw(window);
	snake.draw(window);

	sf::Text scoreText(font, "Score: " + std::to_string(score), 20);
	scoreText.setFillColor(sf::Color::White);
	scoreText.setPosition(sf::Vector2f(8.f, 4.f));
	window.draw(scoreText);
}

bool Game::isGameOver() const 
{
	return gameOver;
}

int Game::getScore() const 
{
	return score;

}

Game::Game(std::mt19937 random) : score(0), gameOver(false), snake(Snake(100, 100)), food(Food(100,200, false)), random(random) {}

void Game::handleInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
		snake.changeDirection(0);
    else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
        snake.changeDirection(2);
    else if (key == sf::Keyboard::Key::Left || key == sf::Keyboard::Key::A)
        snake.changeDirection(3);
    else if (key == sf::Keyboard::Key::Right || key == sf::Keyboard::Key::D)
        snake.changeDirection(1);
}

Snake& Game::getSnake() 
{
	return snake;
}

Food& Game::getFood() 
{
	return food;
}

void Game::update()
{
	snake.move();

	if (snake.checkCollision(food.getX(), food.getY(), COLLISION::FOOD))
	{
		increaseScore(food.isSpecial() ? 5 : 1);
		snake.grow();

		int newX = (this->random() % 30) * 20;
		int newY = (this->random() % 30) * 20;

		while (snake.checkCollision(newX, newY))
		{
			newX = (this->random() % 30) * 20;
			newY = (this->random() % 30) * 20;
		}
		food.setPosition(newX, newY);
		food.setSpecial((this->random() % 5) == 0);
	}
	else if (snake.checkCollision(snake.getHeadX(), snake.getHeadY(), COLLISION::SELF) && snake.getSize() > 1)
	{
		setGameOver(true);
	}
}