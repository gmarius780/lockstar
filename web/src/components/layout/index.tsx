import React, { ReactChild, useState } from 'react'
import { Link, useLocation, matchPath } from 'react-router-dom'
import AppBar from '@material-ui/core/AppBar'
import CssBaseline from '@material-ui/core/CssBaseline'
import Divider from '@material-ui/core/Divider'
import Drawer from '@material-ui/core/Drawer'
import Hidden from '@material-ui/core/Hidden'
import IconButton from '@material-ui/core/IconButton'
import List from '@material-ui/core/List'
import ListItem from '@material-ui/core/ListItem'
import ListItemIcon from '@material-ui/core/ListItemIcon'
import ListItemText from '@material-ui/core/ListItemText'
import MenuIcon from '@material-ui/icons/Menu'
import Toolbar from '@material-ui/core/Toolbar'
import Typography from '@material-ui/core/Typography'
import { Theme, makeStyles, useTheme, createStyles } from '@material-ui/core/styles'

import routes from '../../routes'
import Logo from './logo'

const drawerWidth = 240

const useStyles = makeStyles((theme: Theme) => createStyles({
  root: {
    display: 'flex',
  },
  drawer: {
    [theme.breakpoints.up('sm')]: {
      width: drawerWidth,
      flexShrink: 0,
    },
  },
  appBar: {
    [theme.breakpoints.up('sm')]: {
      width: `calc(100% - ${drawerWidth}px)`,
      marginLeft: drawerWidth,
    },
    backgroundColor: '#282c34',
  },
  menuButton: {
    marginRight: theme.spacing(2),
    [theme.breakpoints.up('sm')]: {
      display: 'none',
    },
  },
  // necessary for content to be below app bar
  toolbar: theme.mixins.toolbar,
  drawerPaper: {
    width: drawerWidth,
  },
  content: {
    flexGrow: 1,
    padding: theme.spacing(3),
  },
  navListLink: {
    textDecoration: 'none',
  },
  navListItemText: {
    color: '#000',
  },
}))

interface CustomListItemProps {
  path: string,
  icon: any
  selected: boolean,
  onClick: () => void,
  label?: string,
  divider?: boolean,
}

const CustomListItem = ({ label, path, divider, selected, onClick, icon: Icon }: CustomListItemProps) => {
  const classes = useStyles()

  return (
    <Link to={path} className={classes.navListLink} onClick={onClick}>
      <ListItem divider={divider} selected={selected} button>
        <ListItemIcon>
          <Icon />
        </ListItemIcon>
        <ListItemText
          classes={{
            root: classes.navListItemText,
          }}
          primary={label}
        />
      </ListItem>
    </Link>
  )
}

interface Props {
  children: ReactChild
}

export default ({ children }: Props) => {
  const classes = useStyles()
  const theme = useTheme()
  const location = useLocation()
  const [mobileOpen, setMobileOpen] = useState(false)

  const handleDrawerToggle = () => setMobileOpen(!mobileOpen)

  let headerTitle = 'Not Found'
  const currentRoute = routes.find(route => {
    if (route.path !== '/' && route.path !== '/*') {
      return location.pathname.startsWith(route.path)
    }

    return route.path === location.pathname
  })
  if (currentRoute && currentRoute.label) {
    headerTitle = currentRoute.label
  }

  const drawer = (
    <div>
      <div className={classes.toolbar}>
        <Link to={'/'} className={classes.navListLink} onClick={() => setMobileOpen(false)}>
          <Logo />
        </Link>
      </div>
      <Divider />
      <List>
        {
          routes.filter(route => route.sidebar).map((route) => (
            <CustomListItem
              key={route.path}
              label={route.label}
              path={route.path}
              icon={route.icon}
              divider={route.divider}
              selected={!!matchPath(location.pathname, { path: route.path, exact: route.exact })}
              onClick={() => setMobileOpen(false)}
            />
          ))
        }
      </List>
    </div>
  )

  return (
    <div className={classes.root}>
      <CssBaseline />
      <AppBar position="fixed" className={classes.appBar}>
        <Toolbar>
          <IconButton
            color="inherit"
            aria-label="open drawer"
            edge="start"
            onClick={handleDrawerToggle}
            className={classes.menuButton}
          >
            <MenuIcon />
          </IconButton>
          <Typography variant="h6" noWrap>
            {headerTitle}
          </Typography>
        </Toolbar>
      </AppBar>
      <nav className={classes.drawer} aria-label="mailbox folders">
        {/* The implementation can be swapped with js to avoid SEO duplication of links. */}
        <Hidden smUp implementation="css">
          <Drawer
            variant="temporary"
            anchor={theme.direction === 'rtl' ? 'right' : 'left'}
            open={mobileOpen}
            onClose={handleDrawerToggle}
            classes={{
              paper: classes.drawerPaper,
            }}
            ModalProps={{
              keepMounted: true, // Better open performance on mobile.
            }}
          >
            {drawer}
          </Drawer>
        </Hidden>
        <Hidden xsDown implementation="css">
          <Drawer
            classes={{
              paper: classes.drawerPaper,
            }}
            variant="permanent"
            open
          >
            {drawer}
          </Drawer>
        </Hidden>
      </nav>
      <main className={classes.content}>
        <div className={classes.toolbar} />
        {children}
      </main>
    </div>
  )
}
