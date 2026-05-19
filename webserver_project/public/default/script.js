document.addEventListener('DOMContentLoaded', () => {
    const greetingElement = document.getElementById('dynamic-greeting');
    const exploreBtn = document.getElementById('explore-btn');

    // 1. Dynamic Greeting based on your system time
    const currentHour = new Date().getHours();
    let greetingText = 'Welcome!';

    if (currentHour < 12) {
        greetingText = 'Good Morning & Welcome!';
    } else if (currentHour < 18) {
        greetingText = 'Good Afternoon & Welcome!';
    } else {
        greetingText = 'Good Evening & Welcome!';
    }

    greetingElement.textContent = greetingText;

    // 2. Simple Button Click Event
    exploreBtn.addEventListener('click', () => {
        alert('Thanks for clicking! You can easily wire this button up to point to another page or section.');
    });
});