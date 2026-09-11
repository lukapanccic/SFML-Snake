#include "menu.hpp"

void centerOrigin(sf::Text& text)
{
	sf::FloatRect bounds = text.getLocalBounds();
	text.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
}

Menu::Menu(std::vector<std::string> items) : items(std::move(items)), selectedIndex(0) {}

void Menu::moveUp()
{
	selectedIndex = (selectedIndex - 1 + (int)items.size()) % (int)items.size();
}

void Menu::moveDown()
{
	selectedIndex = (selectedIndex + 1) % (int)items.size();
}

int Menu::getSelectedIndex() const
{
	return selectedIndex;
}

std::vector<sf::FloatRect> Menu::computeLayout(sf::RenderWindow& window) const
{
	std::vector<sf::FloatRect> bounds;

	const float buttonWidth = 240.f;
	const float buttonHeight = 50.f;
	const float spacing = 20.f;

	sf::Vector2u windowSize = window.getSize();
	float totalHeight = items.size() * buttonHeight + (items.size() - 1) * spacing;
	float startY = ((float)windowSize.y - totalHeight) / 2.f;
	float x = ((float)windowSize.x - buttonWidth) / 2.f;

	for (size_t i = 0; i < items.size(); ++i)
	{
		float y = startY + i * (buttonHeight + spacing);
		bounds.push_back(sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(buttonWidth, buttonHeight)));
	}

	return bounds;
}

void Menu::draw(sf::RenderWindow& window, const sf::Font& font, const std::string& title)
{
	std::vector<sf::FloatRect> bounds = computeLayout(window);

	if (!title.empty())
	{
		sf::Text titleText(font, title, 40);
		titleText.setFillColor(sf::Color::White);
		centerOrigin(titleText);
		titleText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, bounds.front().position.y - 60.f));
		window.draw(titleText);
	}

	for (size_t i = 0; i < items.size(); ++i)
	{
		sf::RectangleShape button(bounds[i].size);
		button.setPosition(bounds[i].position);
		button.setFillColor((int)i == selectedIndex ? sf::Color(100, 100, 100) : sf::Color(60, 60, 60));
		button.setOutlineThickness(2.f);
		button.setOutlineColor((int)i == selectedIndex ? sf::Color::Yellow : sf::Color::White);
		window.draw(button);

		sf::Text label(font, items[i], 24);
		label.setFillColor(sf::Color::White);
		centerOrigin(label);
		label.setPosition(bounds[i].getCenter());
		window.draw(label);
	}
}
