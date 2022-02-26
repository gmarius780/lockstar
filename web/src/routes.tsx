import HomeIcon from '@material-ui/icons/Home'
import DeviceHubIcon from '@material-ui/icons/DeviceHub'
import SettingsIcon from '@material-ui/icons/Settings'
import InfoIcon from '@material-ui/icons/Info'

import HomePage from './pages/home'
import DevicesPage from './pages/devices'
import SettingsPage from './pages/settings'
import AboutPage from './pages/about'
import WelcomePage from './pages/welcome'
import NotFoundPage from './pages/not-found'

interface Route {
  path: string,
  component: any,
  label?: string,
  exact?: boolean,
  sidebar?: boolean,
  divider?: boolean,
  icon?: any,
}

export default [
  {
    path: '/',
    label: 'Home',
    exact: true,
    icon: HomeIcon,
    component: HomePage,
    sidebar: true,
  },
  {
    path: '/devices',
    label: 'Devices',
    divider: true,
    icon: DeviceHubIcon,
    component: DevicesPage,
    sidebar: true,
  },
  {
    path: '/settings',
    label: 'Settings',
    exact: true,
    icon: SettingsIcon,
    component: SettingsPage,
    sidebar: true,
  },
  {
    path: '/about',
    label: 'About',
    exact: true,
    icon: InfoIcon,
    component: AboutPage,
    sidebar: true,
  },
  {
    path: '/welcome',
    label: 'Welcome',
    exact: true,
    component: WelcomePage,
  },
  {
    path: '/*',
    component: NotFoundPage,
  },
] as Route[]
