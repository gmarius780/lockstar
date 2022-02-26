export default function useLocalStorage(key: string, initialValue: string | null = null) {
  if (initialValue) {
    window.localStorage.setItem(key, initialValue)
  }

  const setValue = (value: string) => {
    window.localStorage.setItem(key, value)
  }

  return [window.localStorage.getItem(key) as string, setValue]
}
