/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{vue,js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {},
  },
  plugins: [
    /* eslint-disable no-undef */
    require('@headlessui/tailwindcss'),
    require('@tailwindcss/forms')
  ]
}

